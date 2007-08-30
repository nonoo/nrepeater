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

void CLoop::ProcessAudio()
{
    struct timeval time;
    int n = 0;
    int nFDIn = g_SNDCardIn->GetFDIn();
    fd_set reads;

    FD_ZERO( &reads );
    FD_SET( 0, &reads ); // stdin
    FD_SET( nFDIn, &reads );

    time.tv_sec = 1;
    time.tv_usec = 0;
    if( ( n = select( nFDIn + 1, &reads, NULL, NULL, &time ) ) == -1 )
    {
	g_Log.Error( "select()\n" );
	exit( -1 );
    }

    if( n == 0 )
    {
        g_Log.Debug( "select() timeout\n");
        return;
    }

    if( FD_ISSET(0, &reads) ) // keyboard input
    {
	exit( 0 );
    }

    if( FD_ISSET( nFDIn, &reads) )
    {
	char* pBuffer = g_SNDCardIn->Read();
	g_SNDCardOut->Write( pBuffer );
    }
}

// main loop
void CLoop::Start()
{
    bool fSquelchOff = false;

    for( ;; )
    {
	if( !pin_is_set( SQOFF ) && !fSquelchOff )
	{
	    g_Log.Debug( "squelch off, starting transmission\n" );
	    g_SNDCardIn->Start();
	    fSquelchOff = true;
	}
	if( pin_is_set( SQOFF ) && fSquelchOff )
	{
	    g_Log.Debug( "squelch on\n" );
	    g_SNDCardIn->Stop();
	    fSquelchOff = false;
	}

	if( fSquelchOff )
	{
	    ProcessAudio();
	}
	else
	{
	    usleep( 100 );
	}
    }
}
