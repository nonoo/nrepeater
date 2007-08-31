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

    m_nReceiverPin = 11;
    m_fReceiverLow = true;
    m_nTransmitterPin1 = 2;
    m_nTransmitterPin2 = 3;
}

void CParPort::init()
{
    clear_pin( LP_PIN[m_nReceiverPin] );
    pin_input_mode( LP_PIN[m_nReceiverPin] );
    clear_pin( LP_PIN[m_nReceiverPin] );

    setPTT( false );
    pin_output_mode( LP_PIN[m_nTransmitterPin1] | LP_PIN[m_nTransmitterPin2] );
}

void CParPort::setReceiverPin( int nPin )
{
    m_nReceiverPin = nPin;
}

void CParPort::setReceiverLow( bool fLow )
{
    m_fReceiverLow = fLow;
}

void CParPort::setTransmitterPin1( int nPin )
{
    m_nTransmitterPin1 = nPin;
}

void CParPort::setTransmitterPin2( int nPin )
{
    m_nTransmitterPin2 = nPin;
}

void CParPort::setPTT( bool fState )
{
    change_pin( LP_PIN[m_nTransmitterPin1] | LP_PIN[m_nTransmitterPin2], fState );
}

bool CParPort::isSquelchOff()
{
    if( m_fReceiverLow )
    {
	if( pin_is_set( LP_PIN[m_nReceiverPin] ) )
	{
	    return false;
	}
	else
	{
	    return true;
	}
    }
    else
    {
	if( pin_is_set( LP_PIN[m_nReceiverPin] ) )
	{
	    return true;
	}
	else
	{
	    return false;
	}
    }
}
