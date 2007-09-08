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

#include "Archiver.h"
#include "Main.h"
#include "SettingsFile.h"
#include "Log.h"

#include <time.h>
#include <stdio.h>

using namespace std;

extern CSettingsFile	g_MainConfig;
extern CLog		g_Log;

CArchiver::~CArchiver()
{
    SAFE_DELETE( m_pOgg );
    event2( "\n" );
    event2( "* " + string( PACKAGE_NAME ) + " exiting." );
    fclose( m_pEventFile );
}

void CArchiver::init( int nSampleRate, int nChannels )
{
    m_nSampleRate = nSampleRate;
    m_nChannels = nChannels;
    m_bArchiverEnabled = g_MainConfig.GetInt( "archiver", "enabled", 1 );

    m_nDay = -1;
    m_lArchivedSamples = 0;

    m_pOgg = new COggFileOutStream( 0 );
    m_pEventFile = NULL;

    maintain();
}

void CArchiver::write( short* pData, int nFramesNum )
{
    if( !m_bArchiverEnabled )
    {
	return;
    }

    m_SpeexCodec.encode( pData, nFramesNum );
    m_lArchivedSamples += nFramesNum;
}

// returns current time in ymd format
string CArchiver::currDate()
{
    char tmp[50];
    strftime( tmp, 50, "%y%m%d%S", m_stLocalTime );
    return tmp;
}

// returns with a generated filename
string CArchiver::getLogFileName()
{
    return g_MainConfig.Get( "archiver", "prefix", "log-" ) + currDate();
}

void CArchiver::event( string sEvent )
{
    if( ( !m_bArchiverEnabled ) || ( m_pEventFile == NULL ) )
    {
	return;
    }
    char tmp1[50], tmp2[50];
    strftime( tmp1, 50, "%H:%M:%S", m_stLocalTime );

    // calculating file time
    time_t tt = m_lArchivedSamples / m_nSampleRate;
    struct tm* t_tm = localtime( &tt );
    t_tm->tm_hour--;
    strftime( tmp2, 50, "%H:%M:%S", t_tm );

    fprintf( m_pEventFile, "[%s] [%s] %s\n", tmp2, tmp1, sEvent.c_str() );

    fflush( m_pEventFile );
}

// this is the same as event(), but without filetime display
void CArchiver::event2( string sEvent )
{
    if( ( !m_bArchiverEnabled ) || ( m_pEventFile == NULL ) )
    {
	return;
    }
    char tmp[50];
    strftime( tmp, 50, "%H:%M:%S", m_stLocalTime );
    if( sEvent == "\n" )
    {
	fprintf( m_pEventFile, "\n" );
    }
    else
    {
	fprintf( m_pEventFile, "[%s] %s\n", tmp, sEvent.c_str() );
    }

    fflush( m_pEventFile );
}

// checks if new log files should be started
void CArchiver::maintain()
{
    m_stTime = time( NULL );
    m_stLocalTime = localtime( &m_stTime );
//    if( m_stLocalTime.tm_mday != m_nDay )
//cout << m_stLocalTime->tm_min << ":" << m_stLocalTime->tm_sec << endl;
    if( m_stLocalTime->tm_min != m_nDay )
    {
	// day changed, starting new log files
	g_Log.Debug( "starting new log files\n" );

	m_SpeexCodec.destroy();
	m_pOgg->destroy();

	m_pOgg->init( g_MainConfig.Get( "archiver", "dir", "./" ) + getLogFileName() + ".spx" );
	m_SpeexCodec.initEncode( m_pOgg, m_nSampleRate, m_nChannels, g_MainConfig.GetInt( "archiver", "bitrate", 10000 ) );

	m_lArchivedSamples = 0;

	// creating text logfile
	event2( "\n" );
	event2( "* switching log files.\n" );
	if( m_pEventFile != NULL )
	{
	    fclose( m_pEventFile );
	}
	m_pEventFile = fopen( string( g_MainConfig.Get( "archiver", "dir", "./" ) + getLogFileName() + ".txt" ).c_str() , "w" );
	event2( "* " + string( PACKAGE_NAME ) + " text log for " + getLogFileName() + ".spx" );
	event2( "* speex encoder bitrate: " + g_MainConfig.Get( "archiver", "bitrate", "10000" ) + " bps" );
	char tmp[50];
	strftime( tmp, 50, "%Y/%m/%d", m_stLocalTime );
	event2( "* date: " + string( tmp ) );
	fprintf( m_pEventFile, "\n[FILETIME] [REALTIME]\n" );
    }
//    m_nDay = m_stLocalTime.tm_mday;
    m_nDay = m_stLocalTime->tm_min;
}
