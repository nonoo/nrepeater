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

#include "Log.h"
#include "ParPort.h"

extern CLog g_Log;

CParPort::CParPort( int port )
{
    if ( pin_init_user( port ) < 0 )
    {
	char tmp[50];
	sprintf( tmp, "failed to open LPT port 0x%x\n", port );
	g_Log.Error( tmp );
	exit( -1 );
    }

    clear_pin( SQOFF );
    pin_input_mode( SQOFF );
    clear_pin( SQOFF );

    clear_pin( PTT );
}
