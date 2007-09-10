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
extern CSettingsFile	g_MainConfig;
extern bool		g_fTerminate;
extern CLoop		g_Loop;
extern CArchiver	g_Archiver;

void onSIGALRM( int )
{
    g_ParPort->setPTT( false );
    g_SNDCardOut->stop();

    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "beep finished, transmission stopped.\n" );
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

    // initializing beeps
    if( g_MainConfig.getInt( "beeps", "rogerbeep_enabled", 1 ) )
    {
	// initializing roger beep
	if( m_RogerBeep.loadToMemory( g_MainConfig.get( "beeps", "file", "beep.wav" ) ) )
	{
	    m_RogerBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "roger beep file loaded into memory\n" );
	}
    }
    if( g_MainConfig.getInt( "beeps", "ackbeep_enabled", 1 ) )
    {
	// initializing roger beep
	if( m_AckBeep.loadToMemory( g_MainConfig.get( "beeps", "ackbeep_file", "ackbeep.wav" ) ) )
	{
	    m_AckBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "ack beep file loaded into memory\n" );
	}
    }
    if( g_MainConfig.getInt( "beeps", "failbeep_enabled", 1 ) )
    {
	// initializing roger beep
	if( m_AckBeep.loadToMemory( g_MainConfig.get( "beeps", "failbeep_file", "failbeep.wav" ) ) )
	{
	    m_AckBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "fail beep file loaded into memory\n" );
	}
    }
    m_nBeepDelay = g_MainConfig.getInt( "beeps", "delay", 2 );
    m_nPlayBeepTime = 0;

    signal( SIGALRM, onSIGALRM );

    if( g_MainConfig.getInt( "compressor", "enabled", 0 ) )
    {
	m_Compressor.init( g_SNDCardOut->getSampleRate(), g_SNDCardOut->getBufferSize() );
    }
    if( g_MainConfig.getInt( "archiver", "enabled", 0 ) || g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	// only initialize resampler if archiver or dtmf decoder is enabled
	m_Resampler.init( ( 1.0 * SPEEX_SAMPLERATE ) / g_SNDCardIn->getSampleRate(), g_SNDCardOut->getChannelNum() );
    }
    if( g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	m_DTMF.init( SPEEX_SAMPLERATE );
    }

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
	    m_bPlayingBeep = false;

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

	    // initializing status booleans
	    m_bPlayRogerBeep = m_RogerBeep.isLoaded();
	    m_bPlayAckBeep = m_bPlayFailBeep = false;

	    // querying decoded DTMF sequence
	    m_pszDTMFDecoded = m_DTMF.finishDecoding();
	    if( m_pszDTMFDecoded != NULL )
	    {
		g_Log.log( CLOG_MSG | CLOG_TO_ARCHIVER, "received DTMF sequence: " + string( m_pszDTMFDecoded ) + "\n" );
		if( m_DTMF.isValidSequence( m_pszDTMFDecoded ) )
		{
		    // sequence valid, playing ack beep
		    if( m_AckBeep.isLoaded() )
		    {
			m_bPlayRogerBeep = false;
			m_bPlayAckBeep = true;

			// setting the time we have to play the beep
			m_nPlayBeepTime = time( NULL ) + m_nBeepDelay;
			m_AckBeep.rewind();
		    }
		}
		else
		{
		    // sequence invalid, playing fail beep
		    if( m_FailBeep.isLoaded() )
		    {
			m_bPlayRogerBeep = false;
			m_bPlayFailBeep = true;

			// setting the time we have to play the beep
			m_nPlayBeepTime = time( NULL ) + m_nBeepDelay;
			m_FailBeep.rewind();
		    }
		}
		m_DTMF.clearSequence();
	    }

	    // do we have to play the roger beep?
	    if( m_bPlayRogerBeep )
	    {
		// setting the time we have to play the beep
		m_nPlayBeepTime = time( NULL ) + m_nBeepDelay;
		m_RogerBeep.rewind();
	    }

	    // if we don't have to play a beep
	    if( !m_bPlayRogerBeep && !m_bPlayAckBeep && !m_bPlayFailBeep )
	    {
		g_SNDCardOut->stop();
		g_ParPort->setPTT( false );

		g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished, transmission stopped.\n" );
	    }
	    else
	    {
		g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished\n" );
	    }
	}

	// playing time of the beep reached
	if( ( m_nPlayBeepTime ) && ( time( NULL ) > m_nPlayBeepTime ) )
	{
	    m_nPlayBeepTime = 0;
	    m_bPlayingBeep = true;

	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playing roger beep\n" );
	}

	// this plays the beep wave sequentially
	if( m_bPlayingBeep )
	{
	    if( m_bPlayRogerBeep )
	    {
		m_pBuffer = m_RogerBeep.play( g_SNDCardOut->getBufferSize(), m_nFramesRead );
	    }
	    if( m_bPlayAckBeep )
	    {
		m_pBuffer = m_AckBeep.play( g_SNDCardOut->getBufferSize(), m_nFramesRead );
	    }
	    if( m_bPlayFailBeep )
	    {
		m_pBuffer = m_FailBeep.play( g_SNDCardOut->getBufferSize(), m_nFramesRead );
	    }

	    if( m_pBuffer == NULL )
	    {
	        // reached the end of the wave
	        m_bPlayingBeep = false;

	        // turning off transmitter after given microseconds
	        // setting up timer
	        setTransmitTimeout( g_MainConfig.getInt( "beeps", "delayafter", 250000 ) );
	    }
	    else
	    {
	        // playing beep
		g_SNDCardOut->write( m_pBuffer, m_nFramesRead );

		// resampling
		m_nResampledFramesNum = 0;
		short* pResampledData = m_Resampler.resample( m_pBuffer, m_nFramesRead, m_nResampledFramesNum );

		// archiving
	    	g_Archiver.write( pResampledData, m_nResampledFramesNum );
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
	    m_nCompressedFramesNum = 0;
	    short* pCompOut = m_Compressor.process( m_pBuffer, m_nFramesRead, m_nCompressedFramesNum );

	    // playing processed samples
	    g_SNDCardOut->write( pCompOut, m_nCompressedFramesNum );

	    // resampling
	    m_nResampledFramesNum = 0;
	    short* pResampledData = m_Resampler.resample( pCompOut, m_nCompressedFramesNum, m_nResampledFramesNum );

	    // DTMF decoding
	    m_DTMF.process( pResampledData, m_nResampledFramesNum );

	    // archiving
	    g_Archiver.write( pResampledData, m_nResampledFramesNum );
	}
    }
}
