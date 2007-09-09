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
#include "Archiver.h"

#include <iostream>
#include <string>
#include <time.h>

using namespace std;

extern CArchiver g_Archiver;

CLog::CLog()
{
    m_nScreenLogLevel = LOGLEVEL_NORMAL;
    m_nSysLogLevel = LOGLEVEL_NORMAL;
}


void CLog::log( int nFlags, string msg )
{
    bool bDispScreen = false;
    bool bDispSys = false;

    if( nFlags & LOG_MSG )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NONE )
	{
	    bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NONE )
	{
	    bDispSys = true;
	}
    }

    if( nFlags & LOG_ERROR )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NONE )
	{
	    bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NONE )
	{
	    bDispSys = true;
	}
    }

    if( nFlags & LOG_WARNING )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NONE )
	{
	    bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NONE )
	{
	    bDispSys = true;
	}
    }

    if( nFlags & LOG_DEBUG )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NORMAL )
	{
	    bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NORMAL )
	{
	    bDispSys = true;
	}
    }

    if( nFlags & LOG_DEBUG_EXTREME )
    {
	if( m_nScreenLogLevel > LOGLEVEL_DEBUG )
	{
	    bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_DEBUG )
	{
	    bDispSys = true;
	}
    }


    if( nFlags & LOG_ERROR )
    {
	msg = "Error: " + msg;
    }
    
    if( nFlags & LOG_WARNING )
    {
	msg = "Warning: " + msg;
    }


    if( bDispScreen )
    {
	if( nFlags & LOG_NO_TIME_DISPLAY )
	{
	    cout << msg;
	}
	else
	{
	    cout << CurrTime() << msg;
	}
    }
    if( bDispSys )
    {
	// todo: add syslog code here
    }

    if( nFlags & LOG_TO_ARCHIVER )
    {
	g_Archiver.event( msg );
    }
}

void CLog::setScreenLogLevel( int nLogLevel )
{
    m_nScreenLogLevel = nLogLevel;
}

void CLog::setSysLogLevel( int nLogLevel )
{
    m_nSysLogLevel = nLogLevel;
}

// returns the time in [H:m:s] format
string CLog::CurrTime()
{
    char tmp[50];
    time_t t = time( NULL );
    struct tm *tmpt = localtime( &t );
    strftime( tmp, 50, "%T", tmpt );
    return "[" + string( tmp ) + "] ";
}
