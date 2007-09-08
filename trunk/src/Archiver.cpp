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

using namespace std;

extern CSettingsFile	g_MainConfig;
extern CLog		g_Log;

CArchiver::~CArchiver()
{
    SAFE_DELETE( m_pOgg );
}

void CArchiver::init( int nSampleRate, int nChannels )
{
    m_nSampleRate = nSampleRate;
    m_nChannels = nChannels;
    m_bArchiverEnabled = g_MainConfig.GetInt( "archiver", "enabled", 1 );

    m_stTime = time( NULL );
    m_stLocalTime = localtime( &m_stTime );
    m_nDay = m_stLocalTime->tm_min;

    m_pOgg = new COggFileOutStream( 0 );
    m_pOgg->init( string( g_MainConfig.Get( "archiver", "dir", "./" ) + g_MainConfig.Get( "archiver", "prefix", "log-" ) + currDate() + ".spx" ).c_str() );
    m_SpeexCodec.initEncode( m_pOgg, m_nSampleRate, m_nChannels, g_MainConfig.GetInt( "archiver", "bitrate", 10000 ) );
}

void CArchiver::write( short* pData, int nFramesNum )
{
    if( !m_bArchiverEnabled )
    {
	return;
    }

    m_stTime = time( NULL );
    m_stLocalTime = localtime( &m_stTime );
//    if( m_stLocalTime.tm_mday != m_nDay )
cout << m_stLocalTime->tm_min << ":" << m_stLocalTime->tm_sec << endl;
    if( m_stLocalTime->tm_min != m_nDay )
    {
	g_Log.Debug( "starting new log files\n" );

	// day changed, starting new log files
	m_SpeexCodec.destroy();
	m_pOgg->destroy();

	m_pOgg->init( string( g_MainConfig.Get( "archiver", "dir", "./" ) + g_MainConfig.Get( "archiver", "prefix", "log-" ) + currDate() + ".spx" ).c_str() );
	m_SpeexCodec.initEncode( m_pOgg, m_nSampleRate, m_nChannels, g_MainConfig.GetInt( "archiver", "bitrate", 10000 ) );
    }
//    m_nDay = m_stLocalTime.tm_mday;
    m_nDay = m_stLocalTime->tm_min;

    m_SpeexCodec.encode( pData, nFramesNum );
}

// returns current time in ymd format
string CArchiver::currDate()
{
    char tmp[50];
    strftime( tmp, 50, "%y%m%d%S", m_stLocalTime );
    return tmp;
}
