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

    // beep playing hasn't been started yet, but the delay after receiving is over
    if( g_Loop.m_bPlayingBeepStart )
    {
	// starting beep playing
	g_Loop.m_bPlayingBeep = true;
	g_Loop.m_bPlayingBeepStart = false;

	if( g_Loop.m_bPlayRogerBeep )
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playing roger beep\n" );
	}
	if( g_Loop.m_bPlayAckBeep )
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playing ack beep\n" );
	}
	if( g_Loop.m_bPlayFailBeep )
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playing fail beep\n" );
	}
	return;
    }

    if( g_Loop.m_bPlayAckBeep )
    {
	// ack beep has just finished playing, delay after ack beep is over
	g_Loop.m_bPlayAckBeep = false;
	g_Log.log( CLOG_DEBUG, "beep finished\n" );

	g_Loop.m_bDTMFProcessingSuccess = g_Loop.m_DTMF.processSequence( g_Loop.m_pszDTMFDecoded );

	g_Loop.m_DTMF.clearSequence();
	// turning off transmitter after given microseconds
	// setting up timer
	g_Loop.setAlarm( g_MainConfig.getInt( "dtmf", "delay_after_action", 250 ) );
	return;
    }

    if( g_Loop.m_bProcessingDTMFAction )
    {
	// processing DTMF action is over, delay after action is over
	g_Loop.m_bProcessingDTMFAction = false;
	g_SNDCardOut->stop();
	if( g_Loop.m_bDTMFProcessingSuccess )
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "action finished successfully\n" );

	    // playing roger beep if needed
	    g_Loop.m_bPlayRogerBeep = g_Loop.m_RogerBeep.isLoaded();
	    if( g_Loop.m_bPlayRogerBeep )
	    {
		g_Loop.m_bPlayingBeepStart = true;
		g_Loop.m_RogerBeep.rewind();
		//setAlarm( g_MainConfig.getInt( "beeps", "delay_rogerbeep", 1000 ) );
		onSIGALRM( 0 );
	    }
	}
	else
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "action failed\n" );

	    // playing fail beep if needed
	    g_Loop.m_bPlayFailBeep = g_Loop.m_FailBeep.isLoaded();
	    if( g_Loop.m_bPlayFailBeep )
	    {
		g_Loop.m_bPlayingBeepStart = true;
		g_Loop.m_FailBeep.rewind();
		//setAlarm( g_MainConfig.getInt( "beeps", "delay_rogerbeep", 1000 ) );
		onSIGALRM( 0 );
	    }
	}

	return;
    }

    // the delay after the beep is over
    g_SNDCardOut->stop();
    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "beep finished, transmission stopped.\n" );
}

void CLoop::setAlarm( int nMilliSecs )
{
    if( nMilliSecs == 0 )
    {
	// setitimer() does nothing when nMilliSecs == 0
	onSIGALRM( 0 );
	return;
    }

    struct itimerval rttimer;
    struct itimerval old_rttimer;

    rttimer.it_value.tv_sec = 0;
    rttimer.it_value.tv_usec = nMilliSecs * 1000;
    rttimer.it_interval.tv_sec = 0;
    rttimer.it_interval.tv_usec = 0;
    setitimer( ITIMER_REAL, &rttimer, &old_rttimer );
}

void CLoop::clearAlarm()
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
    m_bProcessingDTMFAction = false;
    m_bPlayingBeep = false;
    m_bPlayingBeepStart = false;

    // initializing beeps
    if( g_MainConfig.getInt( "beeps", "rogerbeep_enabled", 1 ) )
    {
	// initializing roger beep
	if( m_RogerBeep.loadToMemory( g_MainConfig.get( "beeps", "rogerbeep_file", "beep.wav" ) ) )
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
	if( m_FailBeep.loadToMemory( g_MainConfig.get( "beeps", "failbeep_file", "failbeep.wav" ) ) )
	{
	    m_AckBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "fail beep file loaded into memory\n" );
	}
    }

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
	if( g_ParPort->isSquelchOff() && !fSquelchOff && !m_bProcessingDTMFAction )
	{
	    clearAlarm();

	    g_SNDCardIn->start();
	    fSquelchOff = true;

	    g_SNDCardOut->stop();
	    g_SNDCardOut->start();
	    g_ParPort->setPTT( true );

	    m_bPlayingBeep = false;
	    m_bPlayingBeepStart = false;

	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving transmission, transmitting started\n" );
	}

	// receiver stopped receiving
	if( !g_ParPort->isSquelchOff() && fSquelchOff && !m_bProcessingDTMFAction )
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
		if( m_DTMF.isValidSequence( m_pszDTMFDecoded ) )
		{
		    g_Log.log( CLOG_MSG | CLOG_TO_ARCHIVER, "received valid DTMF sequence: " + string( m_pszDTMFDecoded ) + "\n" );
		    // sequence valid, playing ack beep
		    if( m_AckBeep.isLoaded() )
		    {
			m_bPlayRogerBeep = false;
			m_bPlayAckBeep = true;
			m_bProcessingDTMFAction = true;

			// setting up a timer that will enable playing beeps after the given delay
			m_bPlayingBeepStart = true;
			setAlarm( g_MainConfig.getInt( "beeps", "delay_ackbeep", 0 ) );
			m_AckBeep.rewind();
		    }
		    else
		    {
			// we don't have to play the ack beep, starting processing sequence
			m_DTMF.processSequence( g_Loop.m_pszDTMFDecoded );

			m_DTMF.clearSequence();
			m_bPlayRogerBeep = false;
			m_bPlayAckBeep = false;
			m_bProcessingDTMFAction = true;
			// turning off transmitter after given microseconds
			// setting up timer
			setAlarm( g_MainConfig.getInt( "dtmf", "delay_after_action", 250 ) );
			continue;
		    }
		}
		else
		{
		    g_Log.log( CLOG_MSG | CLOG_TO_ARCHIVER, "received invalid DTMF sequence (" + string( m_pszDTMFDecoded ) + ")\n" );
		    // sequence invalid, playing fail beep
		    if( m_FailBeep.isLoaded() )
		    {
			m_bPlayRogerBeep = false;
			m_bPlayFailBeep = true;

			// setting up a timer that will enable playing beeps after the given delay
			m_bPlayingBeepStart = true;
			setAlarm( g_MainConfig.getInt( "beeps", "delay_failbeep", 0 ) );
			m_FailBeep.rewind();
		    }
		    m_DTMF.clearSequence();
		}
	    }

	    // do we have to play the roger beep?
	    if( m_bPlayRogerBeep )
	    {
		// setting up a timer that will enable playing beeps after the given delay
		m_bPlayingBeepStart = true;
		setAlarm( g_MainConfig.getInt( "beeps", "delay_rogerbeep", 1000 ) );
		//g_Archiver.writeSilence( g_MainConfig.getInt( "beeps", "delay_rogerbeep", 1000 ) );
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
		int nMSecs = 0;
		if( m_bPlayRogerBeep )
		{
		    nMSecs = g_MainConfig.getInt( "beeps", "delay_after_rogerbeep", 250 );
		}
		if( m_bPlayAckBeep )
		{
		    nMSecs = g_MainConfig.getInt( "beeps", "delay_after_ackbeep", 250 );
		}
		if( m_bPlayFailBeep )
		{
		    nMSecs = g_MainConfig.getInt( "beeps", "delay_after_failbeep", 250 );
		}
	    	setAlarm( nMSecs );
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
	    m_pCompOut = m_Compressor.process( m_pBuffer, m_nFramesRead, m_nCompressedFramesNum );

	    // playing samples
	    g_SNDCardOut->write( m_pCompOut, m_nCompressedFramesNum );

	    // resampling
	    m_nResampledFramesNum = 0;
	    m_pResampledData = m_Resampler.resample( m_pCompOut, m_nCompressedFramesNum, m_nResampledFramesNum );

	    // DTMF decoding
	    m_DTMF.process( m_pResampledData, m_nResampledFramesNum );

	    // archiving
	    g_Archiver.write( m_pResampledData, m_nResampledFramesNum );
	}
    }
}
