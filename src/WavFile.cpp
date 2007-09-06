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

#include <math.h>

#include "WavFile.h"
#include "Log.h"
#include "Main.h"

using namespace std;

extern CLog g_Log;

CWavFile::CWavFile()
{
    m_pWave = NULL;
    m_pSNDFILE = NULL;
    m_nSeek = 0;
}

void CWavFile::openForWrite( string sFile, int nSampleRate, int nChannels, int nFormat )
{
    close();

    memset( &m_SFINFO, 0, sizeof( m_SFINFO ) );
    m_SFINFO.samplerate = nSampleRate;
    m_SFINFO.channels = nChannels;
    m_SFINFO.format = nFormat;
    if( ( m_pSNDFILE = sf_open( sFile.c_str(), SFM_WRITE, &m_SFINFO ) ) == NULL )
    {
	char errstr[100];
	sf_error_str( m_pSNDFILE, errstr, 100 );
	g_Log.Warning( "can't open wav file for writing " + sFile + ": " + errstr + "\n" );
    }

    sf_set_string( m_pSNDFILE, SF_STR_SOFTWARE, PACKAGE_STRING );
}

bool CWavFile::isOpened()
{
    return ( m_pSNDFILE != NULL ? true : false );
}

int CWavFile::write( short* pData, int nFramesNum )
{
    if( m_pSNDFILE == NULL )
    {
	return -1;
    }

    return sf_write_short( m_pSNDFILE, pData, nFramesNum );
}

int CWavFile::write( char* pData, int nBytesNum )
{
    if( m_pSNDFILE == NULL )
    {
	return -1;
    }

    return sf_write_raw( m_pSNDFILE, pData, nBytesNum );
}

void CWavFile::loadToMemory( string sFile )
{
    close();

    memset( &m_SFINFO, 0, sizeof( m_SFINFO ) );
    if( ( m_pSNDFILE = sf_open( sFile.c_str(), SFM_READ, &m_SFINFO ) ) == NULL )
    {
	char errstr[100];
	sf_error_str( m_pSNDFILE, errstr, 100 );
	g_Log.Warning( "can't open wav file " + sFile + ": " + errstr + "\n" );
	return;
    }

    // loading wave data to memory
    m_pWave = new short[ m_SFINFO.frames + 200 ]; // without + 200 there's a SIGSEGV when freeing
    if( sf_read_short( m_pSNDFILE, m_pWave, m_SFINFO.frames ) < m_SFINFO.frames )
    {
	char errstr[100];
	sf_error_str( m_pSNDFILE, errstr, 100 );
	g_Log.Warning( "can't load wav file " + sFile + ": " + errstr + "\n" );
	SAFE_DELETE_ARRAY( m_pWave );
	return;
    }

    rewind();
}

bool CWavFile::isLoaded()
{
    return ( m_pWave != NULL ? true : false );
}

void CWavFile::close()
{
    sf_close( m_pSNDFILE );
    SAFE_DELETE_ARRAY( m_pWave );
}

CWavFile::~CWavFile()
{
    close();
}

// seeks to the beginning of the wave pointer
void CWavFile::rewind()
{
    m_nSeek = 0;
}

// nBufferSize: available sound buffer
// nFramesRead: count of frames read to the buffer
// this is called sequentially
short* CWavFile::play( int nBufferSize, int& nFramesRead )
{
    if( m_pWave == NULL )
    {
	return NULL;
    }

    if( m_nSeek >= m_SFINFO.frames )
    {
	// play ended, rewinding
	m_nSeek = 0;
	return NULL;
    }

    if( nBufferSize + m_nSeek > m_SFINFO.frames )
    {
	nFramesRead = m_SFINFO.frames - m_nSeek;
    }
    else
    {
	nFramesRead = nBufferSize;
    }
    int nOldSeek = m_nSeek;
    m_nSeek += nFramesRead;

    return m_pWave + nOldSeek;
}

short* CWavFile::getWaveData( int& nLength )
{
    nLength = m_SFINFO.frames;
    return m_pWave;
}

int CWavFile::getSampleRate()
{
    return m_SFINFO.samplerate;
}

int CWavFile::getChannelNum()
{
    return m_SFINFO.channels;
}

void CWavFile::setVolume( int nPercent )
{
    if( m_pWave == NULL )
    {
	return;
    }

    for( int n=0; n < m_SFINFO.frames; n++ )
    {
	short t = m_pWave[n];
	int dBAtt = ( ( 100 - nPercent ) * 40 ) / 100; // max 40 dB attenuation
	m_pWave[n] = (short)( t * ( 1 / pow( 10, (float)dBAtt / 10 ) ) );
    }
}
