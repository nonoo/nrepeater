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

#include "Loop.h"
#include "ParPort.h"
#include "SNDCard.h"
#include "Log.h"
#include "WavFile.h"
#include "SettingsFile.h"
#include "Main.h"

extern CParPort* g_ParPort;
extern CSNDCard* g_SNDCardIn;
extern CSNDCard* g_SNDCardOut;
extern CLog g_Log;
extern CWavFile* g_RogerBeep;
extern CSettingsFile g_MainConfig;

// main loop
void CLoop::Start()
{
    bool fSquelchOff = false;
    m_nFDIn = g_SNDCardIn->getFDIn();
    m_nSelectRes = -1;

    m_nBeepDelay = g_MainConfig.GetInt( "rogerbeep", "delay", 2 );
    m_nPlayBeepTime = 0;

    g_Log.Debug( "starting main loop\n" );

    for( ;; )
    {
	// receiver started receiving
	if( g_ParPort->isSquelchOff() && !fSquelchOff )
	{
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

	    // do we have to play a roger beep?
	    if( g_RogerBeep == NULL )
	    {
		g_Log.Debug( "transmitter off\n" );
		g_SNDCardOut->Stop();
		g_ParPort->setPTT( false );
	    }
	    else
	    {
		m_nPlayBeepTime = time( NULL ) + m_nBeepDelay;
		g_RogerBeep->init();
	    }
	}

	// playing roger beep if needed
	if( ( m_nPlayBeepTime ) && ( time( NULL ) > m_nPlayBeepTime ) )
	{
	    g_Log.Debug( "playing beep\n" );
	    m_nPlayBeepTime = 0;
	    m_fPlayingBeep = true;
	}
	if( m_fPlayingBeep )
	{
	    m_pBuffer = g_RogerBeep->play( g_SNDCardOut->getBufferSize(), m_nFramesRead );
	    if( m_pBuffer == NULL )
	    {
		// played beep, switching transmitter off
		g_Log.Debug( "beep end\n" );
		m_fPlayingBeep = false;
		g_Log.Debug( "transmitter off\n" );
		g_ParPort->setPTT( false );
	    }
	    else
	    {
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
	    g_SNDCardOut->Write( m_pBuffer, m_nFramesRead );
	}
    }
}
