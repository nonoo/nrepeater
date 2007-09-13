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

extern CParPort*	g_pParPort;
extern CSNDCard*	g_pSNDCardIn;
extern CSNDCard*	g_pSNDCardOut;
extern CLog		g_Log;
extern CSettingsFile	g_MainConfig;
extern bool		g_fTerminate;
extern CLoop*		g_pLoop;
extern CArchiver	g_Archiver;

void onSIGALRM( int )
{
    // beep playing hasn't been started yet, but the delay after receiving is over
    if( g_pLoop->m_bPlayingBeepStart )
    {
	// starting beep playing
	g_pLoop->m_bPlayingBeep = true;
	g_pLoop->m_bPlayingBeepStart = false;

	if( g_pLoop->m_bPlayRogerBeep )
	{
	    g_Log.log( CLOG_DEBUG, "playing roger beep\n" );
	}
	if( g_pLoop->m_bPlayAckBeep )
	{
	    g_Log.log( CLOG_DEBUG, "playing ack beep\n" );
	}
	if( g_pLoop->m_bPlayFailBeep )
	{
	    g_Log.log( CLOG_DEBUG, "playing fail beep\n" );
	}
	return;
    }

    // the delay after the beep is over
    g_pSNDCardOut->stop();
    g_pParPort->setPTT( false );
    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "beep finished, transmission stopped.\n" );
}

CLoop::CLoop()
{
    m_nFDIn = g_pSNDCardIn->getFDIn();
    m_nSelectRes = -1;

    m_bSquelchOff = false;
    m_bPlayingBeep = false;
    m_bPlayingBeepStart = false;

    m_bParrotMode = g_MainConfig.getInt( "parrot", "enabled", 0 );
    m_pParrotBuffer = NULL;
    m_nParrotBufferSize = 0;
    if( m_bParrotMode )
    {
	g_Log.log( CLOG_MSG, "parrot mode enabled.\n" );
    }

    // initializing beeps
    if( g_MainConfig.getInt( "beeps", "rogerbeep_enabled", 1 ) )
    {
	if( m_RogerBeep.loadToMemory( g_MainConfig.get( "beeps", "rogerbeep_file", "beep.wav" ) ) )
	{
	    m_RogerBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "roger beep file loaded into memory\n" );
	}
    }
    if( g_MainConfig.getInt( "beeps", "ackbeep_enabled", 1 ) )
    {
	if( m_AckBeep.loadToMemory( g_MainConfig.get( "beeps", "ackbeep_file", "ackbeep.wav" ) ) )
	{
	    m_AckBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "ack beep file loaded into memory\n" );
	}
    }
    if( g_MainConfig.getInt( "beeps", "failbeep_enabled", 1 ) )
    {
	if( m_FailBeep.loadToMemory( g_MainConfig.get( "beeps", "failbeep_file", "failbeep.wav" ) ) )
	{
	    m_AckBeep.setVolume( g_MainConfig.getInt( "beeps", "volume", 80 ) );
	    g_Log.log( CLOG_DEBUG, "fail beep file loaded into memory\n" );
	}
    }

    signal( SIGALRM, onSIGALRM );

    if( g_MainConfig.getInt( "compressor", "enabled", 0 ) )
    {
	m_Compressor.init( g_pSNDCardOut->getSampleRate(), g_pSNDCardOut->getBufferSize() );
    }
    if( g_MainConfig.getInt( "archiver", "enabled", 0 ) || g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	// only initialize resampler if archiver or dtmf decoder is enabled
	m_Resampler.init( ( (float)SPEEX_SAMPLERATE ) / g_pSNDCardIn->getSampleRate(), g_pSNDCardOut->getChannelNum() );
    }
    if( g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	m_DTMF.init( SPEEX_SAMPLERATE );
    }
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
    g_Log.log( CLOG_MSG, "starting main loop\n" );

    while( !g_fTerminate )
    {
	// receiver started receiving
	if( g_pParPort->isSquelchOff() && !m_bSquelchOff )
	{
	    clearAlarm();

	    g_pSNDCardIn->start();
	    m_bSquelchOff = true;

	    if( m_bParrotMode )
	    {
		if( m_pParrotBuffer == NULL )
		{
		    // parrot buffer hasn't been allocated yet
		    m_nParrotBufferSize = g_MainConfig.getInt( "parrot", "buffer_size", 200 ) * 1024;
		    m_pParrotBuffer = new short[ m_nParrotBufferSize ];
		    if( m_pParrotBuffer == NULL )
		    {
			// memory allocation failed
			g_Log.log( CLOG_ERROR, "not enough memory for parrot buffer, parrot mode disabled.\n" );

			// restarting loop with parrot mode disabled
			m_bParrotMode = false;
			m_bSquelchOff = false;
			continue;
		    }
		}

		m_nParrotBufferPos = 0;
		m_nParrotBufferFree = m_nParrotBufferSize;

		g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving transmission, recording started\n" );
	    }
	    else
	    {
		g_pSNDCardOut->stop();
		g_pSNDCardOut->start();
		g_pParPort->setPTT( true );

		g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving transmission, transmitting started\n" );
	    }

	    m_bPlayingBeep = false;
	    m_bPlayingBeepStart = false;
	    m_bPlayRogerBeep = false;
	    m_bPlayAckBeep = false;
	    m_bPlayFailBeep = false;
	} // endof: receiver started receiving

	// receiver stopped receiving
	if( !g_pParPort->isSquelchOff() && m_bSquelchOff )
	{
	    g_pSNDCardIn->stop();
	    m_bSquelchOff = false;

	    if( g_MainConfig.getInt( "compressor", "enabled", 0 ) )
	    {
		// resetting compressor
		m_Compressor.flush();
	    }

	    if( m_bParrotMode )
	    {
		parrotReceivingOver();
	    }
	    else
	    {
		// querying decoded DTMF sequence
		checkDTMFSequence();

		// do we have to play the roger beep?
		if( m_RogerBeep.isLoaded() )
		{
		    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished\n" );

		    // setting up a timer that will enable playing beeps after the given delay
		    m_bPlayingBeepStart = true;
		    m_bPlayRogerBeep = true;
		    m_RogerBeep.rewind();
		    setAlarm( g_MainConfig.getInt( "beeps", "delay_rogerbeep", 1000 ) );
		    //g_Archiver.writeSilence( g_MainConfig.getInt( "beeps", "delay_rogerbeep", 1000 ) );
		}
		else
		{
		    g_pSNDCardOut->stop();
		    g_pParPort->setPTT( false );

		    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished, transmission stopped.\n" );
		}
	    }
	} // endof: receiver started receiving

	// this plays the beep wave sequentially
	if( m_bPlayingBeep )
	{
	    if( m_bPlayRogerBeep )
	    {
		m_pBuffer = m_RogerBeep.play( g_pSNDCardOut->getBufferSize(), m_nFramesRead );
	    }
	    if( m_bPlayAckBeep )
	    {
		m_pBuffer = m_AckBeep.play( g_pSNDCardOut->getBufferSize(), m_nFramesRead );
	    }
	    if( m_bPlayFailBeep )
	    {
		m_pBuffer = m_FailBeep.play( g_pSNDCardOut->getBufferSize(), m_nFramesRead );
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
		g_pSNDCardOut->write( m_pBuffer, m_nFramesRead );

		// resampling
		m_nResampledFramesNum = 0;
		m_pResampledData = m_Resampler.resample( m_pBuffer, m_nFramesRead, m_nResampledFramesNum );

		// archiving
	    	g_Archiver.write( m_pResampledData, m_nResampledFramesNum );
	    }
	} // endof: wave playing

	g_Archiver.maintain();

	if( !m_bSquelchOff )
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
	    m_pBuffer = g_pSNDCardIn->read( m_nFramesRead );

	    // compressing
	    m_nCompressedFramesNum = 0;
	    m_pCompOut = m_Compressor.process( m_pBuffer, m_nFramesRead, m_nCompressedFramesNum );

	    if( m_bParrotMode )
	    {
		// recording samples to memory
		m_nParrotBufferFree = m_nParrotBufferSize - m_nParrotBufferPos;
		if( m_nParrotBufferFree > m_nCompressedFramesNum )
		{
		    memcpy( m_pParrotBuffer + m_nParrotBufferPos, m_pCompOut, m_nCompressedFramesNum * 2 );
		    m_nParrotBufferPos += m_nCompressedFramesNum;
		}
		else
		{
		    memcpy( m_pParrotBuffer + m_nParrotBufferPos, m_pCompOut, m_nParrotBufferFree * 2 );
		    // parrot buffer is full
		    g_Log.log( CLOG_DEBUG, "parrot buffer has been filled\n" );
		    // interrupting receiving
		    m_bSquelchOff = false;

		    parrotReceivingOver();
		}
	    }
	    else
	    {
		// playing samples
	        g_pSNDCardOut->write( m_pCompOut, m_nCompressedFramesNum );
	    }

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

// called from a DTMF action
bool CLoop::switchParrotMode()
{
    if( m_bParrotMode )
    {
	m_bParrotMode = false;
	g_Log.log( CLOG_MSG | CLOG_TO_ARCHIVER, "parrot mode switched off.\n" );
    }
    else
    {
	if( m_pParrotBuffer == NULL )
	{
	    // parrot buffer hasn't been allocated yet
	    m_nParrotBufferSize = g_MainConfig.getInt( "parrot", "buffer_size", 200 ) * 1024;
	    m_pParrotBuffer = new short[ m_nParrotBufferSize ];
	    if( m_pParrotBuffer == NULL )
	    {
		// memory allocation failed
		g_Log.log( CLOG_ERROR, "not enough memory for parrot buffer, can't enable parrot mode.\n" );
		return false;
	    }
	}
	m_bParrotMode = true;
	g_Log.log( CLOG_MSG | CLOG_TO_ARCHIVER, "parrot mode switched on.\n" );
    }
    // switch was successful
    return true;
}

// checks if a DTMF sequence has been received and starts it's processing
void CLoop::checkDTMFSequence()
{
    m_pszDTMFDecoded = m_DTMF.finishDecoding();
    if( m_pszDTMFDecoded != NULL )
    {
	if( m_DTMF.isValidSequence( m_pszDTMFDecoded ) )
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "received valid DTMF sequence: " + string( m_pszDTMFDecoded ) + "\n" );
	    // sequence valid, playing ack beep
	    if( m_AckBeep.isLoaded() )
	    {
		usleep( g_MainConfig.getInt( "beeps", "delay_ackbeep", 0 ) * 1000 );
		g_Log.log( CLOG_DEBUG, "playing ack beep\n" );
		playWavFileBlocking( m_AckBeep );
		usleep( g_MainConfig.getInt( "beeps", "delay_after_ackbeep", 250 ) * 1000 );
		g_Log.log( CLOG_DEBUG, "playing ack beep finished\n" );
	    }

	    // starting processing sequence
	    m_bDTMFProcessingSuccess = m_DTMF.processSequence( m_pszDTMFDecoded );

	    m_DTMF.clearSequence();
	    usleep( g_MainConfig.getInt( "dtmf", "delay_after_action", 250 ) * 1000 );

	    if( !m_bDTMFProcessingSuccess )
	    {
		// processing failed, playing fail beep if needed
		if( m_FailBeep.isLoaded() )
		{
		    usleep( g_MainConfig.getInt( "beeps", "delay_failbeep", 0 ) * 1000 );
		    g_Log.log( CLOG_DEBUG, "playing fail beep\n" );
		    playWavFileBlocking( m_AckBeep );
		    usleep( g_MainConfig.getInt( "beeps", "delay_after_failbeep", 250 ) * 1000 );
		    g_Log.log( CLOG_DEBUG, "playing fail beep finished\n" );
		}
	    }
	}
	else
	{
	    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "received invalid DTMF sequence (" + string( m_pszDTMFDecoded ) + ")\n" );
	    m_DTMF.clearSequence();
	    // sequence invalid, playing fail beep if needed
	    if( m_FailBeep.isLoaded() )
	    {
		usleep( g_MainConfig.getInt( "beeps", "delay_failbeep", 0 ) * 1000 );
		g_Log.log( CLOG_DEBUG, "playing fail beep\n" );
		playWavFileBlocking( m_FailBeep );
		usleep( g_MainConfig.getInt( "beeps", "delay_after_failbeep", 250 ) * 1000 );
		g_Log.log( CLOG_DEBUG, "playing fail beep finished\n" );
	    }
	}
    }
}

void CLoop::playWavFileBlocking( CWavFile& WavFile )
{
    WavFile.rewind();
    m_pWaveData = WavFile.play( m_nWaveDataLength );
    g_pSNDCardOut->write( m_pWaveData, m_nWaveDataLength );

    // resampling
    m_nResampledFramesNum = 0;
    m_pResampledData = m_Resampler.resample( m_pWaveData, m_nWaveDataLength, m_nResampledFramesNum );

    // archiving
    g_Archiver.write( m_pResampledData, m_nResampledFramesNum );
}

void CLoop::playWavFileNonBlocking( CWavFile& WavFile )
{
    WavFile.rewind();

    while( ( m_nFramesRead > 0 ) && ( !g_pParPort->isSquelchOff() ) )
    {
	m_pBuffer = WavFile.play( g_pSNDCardOut->getBufferSize(), m_nFramesRead );
	g_pSNDCardOut->write( m_pBuffer, m_nFramesRead );

	// resampling
	m_nResampledFramesNum = 0;
	m_pResampledData = m_Resampler.resample( m_pWaveData, m_nWaveDataLength, m_nResampledFramesNum );

	// archiving
	g_Archiver.write( m_pResampledData, m_nResampledFramesNum );
    }
}

void CLoop::parrotReceivingOver()
{
    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "receiving finished, recording over\n" );

    // switching transmitter on
    g_pParPort->setPTT( true );

    // playing ack beep if needed
    if( m_AckBeep.isLoaded() )
    {
        g_Log.log( CLOG_DEBUG, "playing ack beep\n" );
        playWavFileBlocking( m_AckBeep );
        usleep( g_MainConfig.getInt( "beeps", "delay_after_ackbeep", 250 ) * 1000 );
        g_Log.log( CLOG_DEBUG, "playing ack beep finished\n" );
    }

    g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playing back parrot buffer\n" );
    // playing back parrot buffer
    g_pSNDCardOut->write( m_pParrotBuffer, m_nParrotBufferPos );
    usleep( g_MainConfig.getInt( "parrot", "delay_after_playback", 250 ) * 1000 );

    // querying decoded DTMF sequence
    checkDTMFSequence();

    // do we have to play the roger beep?
    if( m_RogerBeep.isLoaded() )
    {
        g_Log.log( CLOG_DEBUG, "playback of parrot buffer finished, playing roger beep\n" );
        playWavFileBlocking( m_RogerBeep );
        usleep( g_MainConfig.getInt( "beeps", "delay_after_rogerbeep", 250 ) * 1000 );
        g_Log.log( CLOG_DEBUG, "roger beep finished, transmission stopped.\n" );
    }
    else
    {
        g_Log.log( CLOG_DEBUG | CLOG_TO_ARCHIVER, "playback of parrot buffer finished, transmission stopped.\n" );
    }
    g_pSNDCardOut->stop();
    g_pParPort->setPTT( false );
}

CLoop::~CLoop()
{
    SAFE_DELETE_ARRAY( m_pParrotBuffer );
}
