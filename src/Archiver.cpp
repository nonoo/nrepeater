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
#include <sndfile.h>

#include <time.h>

using namespace std;

extern CSettingsFile g_MainConfig;

CArchiver::~CArchiver()
{
    m_WavFile.close();
}

void CArchiver::init( int nSampleRate, int nChannels )
{
    m_nSampleRate = nSampleRate;
    m_nChannels = nChannels;
}

void CArchiver::write( short* pData, int nFramesNum )
{
    if( !m_WavFile.isOpened() )
    {
	//m_WavFile.openForWrite( g_MainConfig.Get( "archiver", "dir", "./" ) + g_MainConfig.Get( "archiver", "prefix", "log-" ) + currDate() + ".wav", m_nSampleRate, m_nChannels, SF_FORMAT_GSM610 );
	m_WavFile.openForWrite( g_MainConfig.Get( "archiver", "dir", "./" ) + g_MainConfig.Get( "archiver", "prefix", "log-" ) + currDate() + ".wav", m_nSampleRate, m_nChannels, SF_FORMAT_WAV );
    }
    m_WavFile.write( pData, nFramesNum );
}

// returns current time in ymd format
string CArchiver::currDate()
{
    char tmp[50];
    time_t t = time( NULL );
    struct tm *tmpt = localtime( &t );
    strftime( tmp, 50, "%y%m%d", tmpt );
    return tmp;
}
