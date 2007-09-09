//  This file is part of nrepeater.
//
//  nrepeater is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  nrepeater is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with nrepeater; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "Main.h"
#include "Loop.h"
#include "ParPort.h"
#include "SNDCard.h"
#include "Log.h"
#include "WavFile.h"
#include "SettingsFile.h"
#include "Archiver.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/time.h>

extern CParPort*	g_ParPort;
extern CSNDCard*	g_SNDCardIn;
extern CSNDCard*	g_SNDCardOut;
extern CLog		g_Log;
extern CWavFile		m_RogerBeep;
extern CSettingsFile	g_MainConfig;
extern bool		g_fTerminate;
extern CLoop		g_Loop;
extern CArchiver	g_Archiver;

void onSIGALRM( int )
{
    g_ParPort->setPTT( false );
    g_SNDCardOut->stop();

    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "roger beep finished, transmission stopped.\n" );
}

void CLoop::setTransmitTimeout( int nMicroSecs )
{
    struct itimerval rttimer;
    struct itimerval old_rttimer;

    rttimer.it_value.tv_sec = 0;
    rttimer.it_value.tv_usec = nMicroSecs;
    rttimer.it_interval.tv_sec = 0;
    rttimer.it_interval.tv_usec = 0;
    setitimer( ITIMER_REAL, &rttimer, &old_rttimer );
}

void CLoop::clearTransmitTimeout()
{
    struct itimerval rttimer;
    struct itimerval old_rttimer;

    rttimer.it_value.tv_sec = 0;
    rttimer.it_value.tv_usec = 0;
    rttimer.it_interval.tv_sec = 0;
    rttimer.it_interval.tv_usec = 0;
    setitimer( ITIMER_REAL, &rttimer, &old_rttimer );
}

// main loop
void CLoop::start()
{
    bool fSquelchOff = false;
    m_nFDIn = g_SNDCardIn->getFDIn();
    m_nSelectRes = -1;

    if( m_RogerBeep.loadToMemory( g_MainConfig.get( "rogerbeep", "file", "beep.wav" ) ) )
    {
	m_RogerBeep.setVolume( g_MainConfig.getInt( "rogerbeep", "volume", 80 ) );
	g_Log.log( CLOG_DEBUG, "roger beep file loaded into memory\n" );
    }
    m_nBeepDelay = g_MainConfig.getInt( "rogerbeep", "delay", 2 );
    m_nPlayBeepTime = 0;

    signal( SIGALRM, onSIGALRM );

    m_Compressor.init( g_SNDCardOut->getSampleRate(), g_SNDCardOut->getBufferSize() );
    m_Resampler.init( ( 1.0 * SPEEX_SAMPLERATE ) / g_SNDCardIn->getSampleRate(), g_SNDCardOut->getChannelNum() );
    m_DTMF.init( SPEEX_SAMPLERATE );

    g_Log.log( CLOG_MSG, "starting main loop\n" );

    while( !g_fTerminate )
    {
	// receiver started receiving
	if( g_ParPort->isSquelchOff() && !fSquelchOff )
	{
	    clearTransmitTimeout();

	    g_SNDCardIn->start();
	    fSquelchOff = true;

	    g_SNDCardOut->stop();
	    g_SNDCardOut->start();
	    g_ParPort->setPTT( true );

	    m_nPlayBeepTime = 0;
	    m_fPlayingBeep = false;

	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving transmission, transmitting started\n" );
	}

	// receiver stopped receiving
	if( !g_ParPort->isSquelchOff() && fSquelchOff )
	{
	    g_SNDCardIn->stop();
	    fSquelchOff = false;

	    if( g_MainConfig.getInt( "compressor", "enabled", 0 ) )
	    {
		// resetting compressor
		m_Compressor.flush();
	    }

	    // do we have to play a roger beep?
	    if( !m_RogerBeep.isLoaded() )
	    {
		g_SNDCardOut->stop();
		g_ParPort->setPTT( false );

		g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished, transmission stopped.\n" );
	    }
	    else
	    {
		m_nPlayBeepTime = time( NULL ) + m_nBeepDelay;
		m_RogerBeep.rewind();

		g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished\n" );
	    }
	}

	// playing roger beep if needed
	if( ( m_nPlayBeepTime ) && ( time( NULL ) > m_nPlayBeepTime ) )
	{
	    m_nPlayBeepTime = 0;
	    m_fPlayingBeep = true;

	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playing roger beep\n" );
	}

	// this plays the roger beep wave sequentially
	if( m_fPlayingBeep )
	{
	    m_pBuffer = m_RogerBeep.play( g_SNDCardOut->getBufferSize(), m_nFramesRead );
	    if( m_pBuffer == NULL )
	    {
		// reached the end of the wave
		m_fPlayingBeep = false;

		// turning off transmitter after given microseconds
		// setting up timer
		setTransmitTimeout( g_MainConfig.getInt( "rogerbeep", "delayafter", 250000 ) );
	    }
	    else
	    {
		// playing roger beep
		g_SNDCardOut->write( m_pBuffer, m_nFramesRead );

		// resampling
		int nResampledFramesNum = 0;
		short* pResampledData = m_Resampler.resample( m_pBuffer, m_nFramesRead, nResampledFramesNum );

		// archiving
	        g_Archiver.write( pResampledData, nResampledFramesNum );
	    }
	}

	g_Archiver.maintain();

	if( !fSquelchOff )
	{
	    // we're not transmitting so we don't need to capture audio
	    usleep( 100 );
	    continue;
	}


	// select stuff may be removed later
        FD_ZERO( &m_fsReads );
	FD_SET( m_nFDIn, &m_fsReads );

	m_tTime.tv_sec = 1;
	m_tTime.tv_usec = 0;

	if( ( m_nSelectRes = select( m_nFDIn + 1, &m_fsReads, NULL, NULL, &m_tTime ) ) == -1 )
	{
	    g_Log.log( CLOG_ERROR, "select()\n" );
	    exit( -1 );
	}

	if( m_nSelectRes == 0 )
	{
    	    g_Log.log( CLOG_DEBUG, "select() timeout\n");
	    continue;
	}

	if( FD_ISSET( m_nFDIn, &m_fsReads) )
	{
	    m_pBuffer = g_SNDCardIn->read( m_nFramesRead );

	    // compressing
	    int nCompressedFramesNum = 0;
	    short* pCompOut = m_Compressor.process( m_pBuffer, m_nFramesRead, nCompressedFramesNum );

	    // playing processed samples
	    g_SNDCardOut->write( pCompOut, nCompressedFramesNum );

	    // resampling
	    int nResampledFramesNum = 0;
	    short* pResampledData = m_Resampler.resample( pCompOut, nCompressedFramesNum, nResampledFramesNum );

	    // DTMF decoding
	    m_DTMF.process( pResampledData, nResampledFramesNum );

	    // archiving
	    g_Archiver.write( pResampledData, nResampledFramesNum );
	}
    }
}
