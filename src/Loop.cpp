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

#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/time.h>

#include "Loop.h"
#include "ParPort.h"
#include "SNDCard.h"
#include "Log.h"
#include "WavFile.h"
#include "SettingsFile.h"
#include "Main.h"

extern CParPort*	g_ParPort;
extern CSNDCard*	g_SNDCardIn;
extern CSNDCard*	g_SNDCardOut;
extern CLog		g_Log;
extern CWavFile		g_RogerBeep;
extern CSettingsFile	g_MainConfig;
extern bool		g_fTerminate;

#define SPEEX_SAMPLERATE 8000

void onSIGALRM( int )
{
    g_Log.Debug( "transmitter off\n" );
    g_ParPort->setPTT( false );
    g_SNDCardOut->Stop();
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
void CLoop::Start()
{
    bool fSquelchOff = false;
    m_nFDIn = g_SNDCardIn->getFDIn();
    m_nSelectRes = -1;

    m_nBeepDelay = g_MainConfig.GetInt( "rogerbeep", "delay", 2 );
    m_nPlayBeepTime = 0;

    g_Log.Debug( "starting main loop\n" );

    signal( SIGALRM, onSIGALRM );

    m_pCompressor = new CCompressor( g_SNDCardOut->getSampleRate(), g_SNDCardOut->getBufferSize() );
    m_Resampler.init( ( 1.0 * SPEEX_SAMPLERATE ) / g_SNDCardIn->getSampleRate(), g_SNDCardOut->getChannelNum() );
    m_Archiver.init( SPEEX_SAMPLERATE, g_SNDCardOut->getChannelNum() );

    while( !g_fTerminate )
    {
	// receiver started receiving
	if( g_ParPort->isSquelchOff() && !fSquelchOff )
	{
	    clearTransmitTimeout();

	    g_Log.Debug( "receiver on\n" );
	    g_SNDCardIn->Start();
	    fSquelchOff = true;

	    g_Log.Debug( "transmitter on\n" );
	    g_SNDCardOut->Stop();
	    g_SNDCardOut->Start();
	    g_ParPort->setPTT( true );

	    m_nPlayBeepTime = 0;
	    m_fPlayingBeep = false;
	}

	// receiver stopped receiving
	if( !g_ParPort->isSquelchOff() && fSquelchOff )
	{
	    g_Log.Debug( "receiver off\n" );
	    g_SNDCardIn->Stop();
	    fSquelchOff = false;

	    if( g_MainConfig.GetInt( "compressor", "enabled", 0 ) )
	    {
		// resetting compressor
		m_pCompressor->flush();
	    }

	    // do we have to play a roger beep?
	    if( !g_RogerBeep.isLoaded() )
	    {
		g_Log.Debug( "transmitter off\n" );
		g_SNDCardOut->Stop();
		g_ParPort->setPTT( false );
	    }
	    else
	    {
		m_nPlayBeepTime = time( NULL ) + m_nBeepDelay;
		g_RogerBeep.rewind();
	    }
	}

	// playing roger beep if needed
	if( ( m_nPlayBeepTime ) && ( time( NULL ) > m_nPlayBeepTime ) )
	{
	    g_Log.Debug( "playing beep\n" );
	    m_nPlayBeepTime = 0;
	    m_fPlayingBeep = true;
	}

	// this plays the roger beep wave sequentially
	if( m_fPlayingBeep )
	{
	    m_pBuffer = g_RogerBeep.play( g_SNDCardOut->getBufferSize(), m_nFramesRead );
	    if( m_pBuffer == NULL )
	    {
		// reached the end of the wave
		g_Log.Debug( "beep end\n" );
		m_fPlayingBeep = false;

		// turning off transmitter after given microseconds
		// setting up timer
		setTransmitTimeout( g_MainConfig.GetInt( "rogerbeep", "delayafter", 250000 ) );
	    }
	    else
	    {
		if( g_MainConfig.GetInt( "archiver", "enabled", 1 ) )
		{
		    m_Archiver.write( m_pBuffer, m_nFramesRead );
		}
		g_SNDCardOut->Write( m_pBuffer, m_nFramesRead );
	    }
	}

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
	    g_Log.Error( "select()\n" );
	    exit( -1 );
	}

	if( m_nSelectRes == 0 )
	{
    	    g_Log.Debug( "select() timeout\n");
	    continue;
	}

	if( FD_ISSET( m_nFDIn, &m_fsReads) )
	{
	    m_pBuffer = g_SNDCardIn->Read( m_nFramesRead );

	    // compressing
	    int nCompressedFramesNum = 0;
	    short* pCompOut = m_pCompressor->process( m_pBuffer, m_nFramesRead, nCompressedFramesNum );

	    // playing processed samples
	    g_SNDCardOut->Write( pCompOut, nCompressedFramesNum );

	    // resampling
	    int nResampledFramesNum = 0;
	    short* pResampledData = m_Resampler.resample( pCompOut, nCompressedFramesNum, nResampledFramesNum );

	    // archiving
	    m_Archiver.write( pResampledData, nResampledFramesNum );
	}
    }
}
