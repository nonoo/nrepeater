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

#include "Main.h"
#include "DTMF.h"
#include "SettingsFile.h"
#include "Log.h"

extern CSettingsFile	g_MainConfig;
extern CLog		g_Log;

void CDTMF::init( int nSampleRate )
{
    m_nSampleRate = nSampleRate;
    m_nSamplesProcessed = m_nPause = m_cDecoded = m_cPrevDecoded = m_nDecodedCharsNum = 0;

    memset( m_caDecodedChars, 0, sizeof( m_caDecodedChars ) );

    m_Goertzel.init( m_nSampleRate );
}

void CDTMF::decodingFinished()
{
    g_Log.log( CLOG_MSG, "DTMF string decoded: " + string( m_caDecodedChars ) + "\n" );

    m_nPause = m_nDecodedCharsNum = 0;
    memset( m_caDecodedChars, 0, sizeof( m_caDecodedChars ) );
}

void CDTMF::process( short* pData, int nFramesNum )
{
    if( !g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	return;
    }

    m_cDecoded = m_Goertzel.process( pData, nFramesNum );

    if( m_cDecoded == 0 )
    {
	// no dtmf tone decoded
        m_nSamplesProcessed = 0;
	m_cPrevDecoded = m_cDecoded = 0;

	m_nPause += nFramesNum;
	if( ( ( (float)m_nPause / m_nSampleRate ) * 1000 > g_MainConfig.getInt( "dtmf", "max_pause", 1000 ) ) && ( m_nDecodedCharsNum > 0 ) )
	{
	    // decoding finished
	    decodingFinished();
	}

	return;
    }

    // protecting m_caDecodedChars from overflow
    if( m_nDecodedCharsNum == sizeof( m_caDecodedChars ) - 1 )
    {
	decodingFinished();
    }

    // beginning of decoding a new tone
    if( m_cPrevDecoded == 0 )
    {
	m_cPrevDecoded = m_cDecoded;
    }

    if( m_cDecoded != m_cPrevDecoded )
    {
        // decoding failed
        m_nSamplesProcessed = 0;
	m_cPrevDecoded = m_cDecoded = 0;
	return;
    }

    m_nSamplesProcessed += nFramesNum;

    if( ( m_nDecodedCharsNum > 0 ) && ( ( (float)m_nPause / m_nSampleRate ) * 1000 < g_MainConfig.getInt( "dtmf", "min_pause", 500 ) ) )
    {
	m_cPrevDecoded = m_cDecoded = 0;
	return;
    }

    if( ( ( (float)m_nSamplesProcessed / m_nSampleRate ) * 1300 > g_MainConfig.getInt( "dtmf", "min_tone_length", 100 ) ) ) // 1300 = little cheating to make configuration easier
    {
	m_caDecodedChars[ m_nDecodedCharsNum++ ] = m_cDecoded;
	char caDbgTmp[500];
	sprintf( caDbgTmp, "DTMF tone decoded: %c\n", m_cDecoded );
	g_Log.log( CLOG_DEBUG, caDbgTmp );
	m_nSamplesProcessed = 0;
	m_cPrevDecoded = m_cDecoded = m_nPause = 0;
    }
    else
    {
	m_cPrevDecoded = m_cDecoded;
    }
}
