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

extern CParPort g_ParPort;
extern CSNDCard* g_SNDCardIn;
extern CSNDCard* g_SNDCardOut;
extern CLog g_Log;

// main loop
void CLoop::Start()
{
    bool fSquelchOff = false;
    m_nFDIn = g_SNDCardIn->GetFDIn();
    m_nSelectRes = -1;

    for( ;; )
    {
	if( !pin_is_set( SQOFF ) && !fSquelchOff )
	{
	    g_Log.Debug( "receiver on\n" );
	    g_SNDCardIn->Start();
	    fSquelchOff = true;

	    g_Log.Debug( "transmitter on\n" );
	    g_SNDCardOut->Start();
	    set_pin( PTT );
	}

	if( pin_is_set( SQOFF ) && fSquelchOff )
	{
	    g_Log.Debug( "receiver off\n" );
	    g_SNDCardIn->Stop();
	    fSquelchOff = false;

	    g_Log.Debug( "transmitter off\n" );
	    g_SNDCardOut->Stop();
	    clear_pin( PTT );
	}


	if( !fSquelchOff )
	{
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
	    m_pBuffer = g_SNDCardIn->Read( m_nReadBytes );
	    g_SNDCardOut->Write( m_pBuffer, m_nReadBytes );
	}
    }
}
