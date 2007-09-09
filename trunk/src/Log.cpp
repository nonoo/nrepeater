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
#include "SettingsFile.h"

#include <iostream>
#include <string>
#include <time.h>
#include <syslog.h>

using namespace std;

extern CArchiver	g_Archiver;
extern CSettingsFile	g_MainConfig;

CLog::CLog()
{
    m_nScreenLogLevel = LOGLEVEL_NORMAL;
    m_nSysLogLevel = LOGLEVEL_NORMAL;
    m_nFileLogLevel = LOGLEVEL_NONE;
}

CLog::~CLog()
{
    if( m_pLogFile )
    {
        fclose( m_pLogFile );
    }
    closelog();
}

void CLog::log( int nFlags, string szMsg )
{
    m_bDispScreen = false;
    m_bDispSys = false;
    m_bDispLogFile = false;

    if( nFlags & CLOG_MSG )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispSys = true;
	}
	if( m_nFileLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispLogFile = true;
	}
    }

    if( nFlags & CLOG_ERROR )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispSys = true;
	}
	if( m_nFileLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispLogFile = true;
	}
    }

    if( nFlags & CLOG_WARNING )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispSys = true;
	}
	if( m_nFileLogLevel > LOGLEVEL_NONE )
	{
	    m_bDispLogFile = true;
	}
    }

    if( nFlags & CLOG_DEBUG )
    {
	if( m_nScreenLogLevel > LOGLEVEL_NORMAL )
	{
	    m_bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_NORMAL )
	{
	    m_bDispSys = true;
	}
	if( m_nFileLogLevel > LOGLEVEL_NORMAL )
	{
	    m_bDispLogFile = true;
	}
    }

    if( nFlags & CLOG_DEBUG_EXTREME )
    {
	if( m_nScreenLogLevel > LOGLEVEL_DEBUG )
	{
	    m_bDispScreen = true;
	}
	if( m_nSysLogLevel > LOGLEVEL_DEBUG )
	{
	    m_bDispSys = true;
	}
	if( m_nFileLogLevel > LOGLEVEL_DEBUG )
	{
	    m_bDispLogFile = true;
	}
    }


    if( m_bDispSys )
    {
	m_nSysFlags = 0;
	if( nFlags & CLOG_ERROR )
	{
	    m_nSysFlags |= LOG_ERR;
	}
	if( nFlags & CLOG_WARNING )
	{
	    m_nSysFlags |= LOG_WARNING;
	}
	if( nFlags & CLOG_DEBUG )
	{
	    m_nSysFlags |= LOG_DEBUG;
	}
	if( nFlags & CLOG_MSG )
	{
	    m_nSysFlags |= LOG_INFO;
	}
	if( !( nFlags & CLOG_NO_TIME_DISPLAY ) )
	{
	    syslog( m_nSysFlags, szMsg.c_str() );
	}
    }


    if( nFlags & CLOG_ERROR )
    {
	szMsg = "Error: " + szMsg;
    }
    if( nFlags & CLOG_WARNING )
    {
	szMsg = "Warning: " + szMsg;
    }


    if( m_bDispScreen )
    {
	if( nFlags & CLOG_NO_TIME_DISPLAY )
	{
	    cout << szMsg;
	}
	else
	{
	    cout << CurrTime() << szMsg;
	}
    }
    if( m_bDispLogFile && m_pLogFile )
    {
	fprintf( m_pLogFile, string( CurrTime() + szMsg ).c_str() );
	fflush( m_pLogFile );
    }

    if( nFlags & CLOG_TO_ARCHIVER )
    {
	g_Archiver.event( szMsg );
    }
}

void CLog::setScreenLogLevel( int nLogLevel )
{
    m_nScreenLogLevel = nLogLevel;
}

void CLog::setSysLogLevel( int nLogLevel )
{
    m_nSysLogLevel = nLogLevel;

    if( m_nSysLogLevel > LOGLEVEL_NONE )
    {
	closelog();

	if( m_nScreenLogLevel == LOGLEVEL_NONE )
	{
	    openlog( PACKAGE_NAME, LOG_CONS | LOG_PID, LOG_DAEMON );
	}
	else
	{
	    openlog( PACKAGE_NAME, LOG_PID, LOG_DAEMON );
	}
    }
}

void CLog::setFileLogLevel( int nLogLevel )
{
    m_nFileLogLevel = nLogLevel;

    if( m_nFileLogLevel > LOGLEVEL_NONE )
    {
	if( m_pLogFile == NULL )
	{
	    // log file already exists?
	    bool bExists = false;
	    m_pLogFile = fopen( g_MainConfig.get( "logging", "logfile", string( PACKAGE_NAME ) + ".log" ).c_str(), "r" );
	    if( m_pLogFile )
	    {
		bExists = true;
		fclose( m_pLogFile );
	    }
	    m_pLogFile = fopen( g_MainConfig.get( "logging", "logfile", string( PACKAGE_NAME ) + ".log" ).c_str(), "a" );
	    if( m_pLogFile == NULL )
	    {
		// can't write to log file
		m_nFileLogLevel = LOGLEVEL_NONE;
		log( CLOG_ERROR, "can't open log file for writing: " + g_MainConfig.get( "logging", "logfile", string( PACKAGE_NAME ) + ".log" ) + "\n" );
		return;
	    }
	    if( bExists )
	    {
		fprintf( m_pLogFile, "\n" );
	    }
	}
    }
    else
    {
	if( m_pLogFile )
	{
	    fclose( m_pLogFile );
	}
    }
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
