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
#include "Compressor.h"
#include "SettingsFile.h"
#include "Log.h"

#include <math.h>

extern CSettingsFile	g_MainConfig;
extern CLog		g_Log;

void CCompressor::init( int nSNDCardRate, int nSNDCardBufferSize )
{
    m_nSampleRate = nSNDCardRate;

    m_bCompressorEnabled = g_MainConfig.GetInt( "compressor", "enabled", 0 );

    // how many frames we need to buffer to delay the audio for the given time
    m_nDelayFramesCount = (int)( ( (float)g_MainConfig.GetInt( "compressor", "lookahead", 0 ) / 1000 ) * m_nSampleRate );
    m_nCurrChunk = 0;
    m_pOut = new short[ nSNDCardBufferSize ];
    m_nOutSize = nSNDCardBufferSize;
    m_nBufferSize = 0;
    m_nOrigRatio = g_MainConfig.GetInt( "compressor", "ratio", 5 );
    m_fRatio = 1;
    m_nMakeUpGain = g_MainConfig.GetInt( "compressor", "makeupgain", 15 );
    m_nThreshold = g_MainConfig.GetInt( "compressor", "threshold", -40 );
    // calculating attack time from ms to frames
    m_nAttackFramesCount = (int)( ( (float)g_MainConfig.GetInt( "compressor", "attacktime", 0 ) / 1000 ) * m_nSampleRate );
    m_nCurrAttackFrameCount = 0;
    // calculating hold time from ms to frames
    m_nHoldFramesCount = (int)( ( (float)g_MainConfig.GetInt( "compressor", "holdtime", 200 ) / 1000 ) * m_nSampleRate );
    m_nCurrHoldFrameCount = 0;
    // calculating release time from ms to frames
    m_nReleaseFramesCount = (int)( ( (float)g_MainConfig.GetInt( "compressor", "releasetime", 100 ) / 1000 ) * m_nSampleRate );
    m_nCurrReleaseFrameCount = m_nReleaseFramesCount;

    // when the receiver starts receiving, start the compressor?
    if( g_MainConfig.GetInt( "compressor", "fadein", 1 ) )
    {
	if( g_MainConfig.GetInt( "compressor", "fadein_hold", 1 ) )
	{
	    m_sState = COMPSTATE_COMPRESSING;
	}
	else
	{
	    m_sState = COMPSTATE_RELEASE;
	}
    }
    else
    {
	m_sState = COMPSTATE_OFF;
    }

    // peak sound value
    m_nPeak = 0;
}

CCompressor::~CCompressor()
{
    destroy();
}

void CCompressor::destroy()
{
    SAFE_DELETE_ARRAY( m_pOut );

    for( vector< tBufferChunk >::iterator it = m_vBuffer.begin(); it != m_vBuffer.end(); it++ )
    {
	SAFE_DELETE_ARRAY( (*it).pData );
    }
}

void CCompressor::flush()
{
    for( vector< tBufferChunk >::iterator it = m_vBuffer.begin(); it != m_vBuffer.end(); it++ )
    {
	memset( (*it).pData, 0, (*it).nSize * 2 );
    }

    // when the receiver starts receiving, start the compressor?
    if( g_MainConfig.GetInt( "compressor", "fadein", 1 ) )
    {
	if( g_MainConfig.GetInt( "compressor", "fadein_hold", 1 ) )
	{
	    m_sState = COMPSTATE_COMPRESSING;
	}
	else
	{
	    m_sState = COMPSTATE_RELEASE;
	}
    }
    else
    {
	m_sState = COMPSTATE_OFF;
    }
}

void CCompressor::calcRatio()
{
    if( m_sState & COMPSTATE_ATTACK )
    {
	if( m_nCurrAttackFrameCount++ < m_nAttackFramesCount )
	{
	    m_fRatio = 1 + ( m_nCurrAttackFrameCount * (float)( m_nOrigRatio - 1 ) ) / m_nAttackFramesCount;
	}
	else
	{
	    // attack finished, holding
	    m_sState = COMPSTATE_COMPRESSING;
	    m_nCurrAttackFrameCount = 0;
	}
	return;
    }

    if( m_sState & COMPSTATE_RELEASE )
    {
	if( m_nCurrReleaseFrameCount-- > 0 )
	{
	    m_fRatio = 1 + ( m_nCurrReleaseFrameCount * (float)( m_nOrigRatio - 1 ) ) / m_nReleaseFramesCount;
	}
	else
	{
	    // release finished, compression over
	    m_sState = COMPSTATE_OFF;
	    m_nCurrReleaseFrameCount = m_nReleaseFramesCount;
	}
	return;
    }

    if( m_sState & COMPSTATE_COMPRESSING )
    {
	if( m_nCurrHoldFrameCount++ >= m_nHoldFramesCount )
	{
	    // hold finished, releasing
	    m_sState = COMPSTATE_COMPRESSING | COMPSTATE_RELEASE;
	    m_nCurrHoldFrameCount = 0;
	}
	else
	{
	    m_fRatio = m_nOrigRatio;
	}
    }
}

int CCompressor::doCompress()
{
    int i;

    // what's the index of the chunk we examine?
    int nExamChunk = m_nCurrChunk + m_vBuffer.size() - 1;
    if( nExamChunk >= (int)m_vBuffer.size() )
    {
	nExamChunk -= m_vBuffer.size();
    }

    if( ( m_sState & COMPSTATE_OFF ) || !( m_sState & COMPSTATE_ATTACK ) )
    {
	// searching for a loud transient
	for( i = 0; i < m_vBuffer[ nExamChunk ].nSize; i++ )
	{
	    if( 20 * log10( abs( m_vBuffer[ nExamChunk ].pData[i] ) ) > 96 + m_nThreshold ) // 96dB dynamic range at 16 bit
	    {
		if( m_sState & COMPSTATE_COMPRESSING )
		{
		    m_nCurrAttackFrameCount = (int)( ( m_fRatio * (float)m_nAttackFramesCount ) / m_nOrigRatio );
		}
		m_sState = COMPSTATE_COMPRESSING | COMPSTATE_ATTACK;
		m_nCurrHoldFrameCount = 0;
		m_nCurrReleaseFrameCount = m_nReleaseFramesCount;
	    }
	}
    }

    // do we have enough space for storing the output?
    if( m_nOutSize < m_vBuffer[ m_nCurrChunk ].nSize )
    {
	SAFE_DELETE_ARRAY( m_pOut );
	m_pOut = new short[ m_vBuffer[ m_nCurrChunk ].nSize ];
	m_nOutSize = m_vBuffer[ m_nCurrChunk ].nSize;
    }

    // copying the audio from the current chunk to the output buffer
    for( i = 0; i < m_vBuffer[ m_nCurrChunk ].nSize; i++ )
    {
	calcRatio();

	// applying compression
	m_pOut[i] = (short)( m_vBuffer[ m_nCurrChunk ].pData[i] / m_fRatio );

	// applying makeup gain
	m_pOut[i] = (short)( m_pOut[i] * pow( 10, (float)m_nMakeUpGain / 10 ) );

	m_nPeak = MAX( m_pOut[i], m_nPeak );
    }

    return m_vBuffer[ m_nCurrChunk ].nSize;
}

// this is called sequentially from the main loop
short* CCompressor::process( short* pBuffer, int nFramesIn, int& nFramesOut )
{
    // data comes in variable-sized chunks from the sound card.
    // we put each incoming chunk into the m_vBuffer vector and
    // add the incoming frame size to the m_nBufferSize.
    // if we got the number of samples needed to delay (m_vBuffer
    // is full == m_nBufferSize is at least the number of samples
    // we need to delay), we jump to the beginning of the vector.

    if( !m_bCompressorEnabled )
    {
	nFramesOut = nFramesIn;
	return pBuffer;
    }

    // do we have a chunk already allocated at the given slot?
    if( (int)m_vBuffer.size() - 1 < m_nCurrChunk )
    {
	// creating a new chunk
	tBufferChunk bcTmp;
	bcTmp.pData = new short[ nFramesIn ];
	memset( bcTmp.pData, 0, nFramesIn * 2 );
	bcTmp.nSize = nFramesIn;
	m_nBufferSize += bcTmp.nSize;
	m_vBuffer.push_back( bcTmp );
    }

    // preparing the output
    nFramesOut = doCompress();





    // does the current chunk have enough memory space for the incoming audio data?
    if( m_vBuffer[ m_nCurrChunk ].nSize < nFramesIn )
    {
	// reallocating
	SAFE_DELETE_ARRAY( m_vBuffer[ m_nCurrChunk ].pData );
	m_vBuffer[ m_nCurrChunk ].pData = new short[ nFramesIn ];
	m_nBufferSize += nFramesIn - m_vBuffer[ m_nCurrChunk ].nSize;
    }

    // making sure m_nBufferSize contains the right buffer size value
    if( m_vBuffer[ m_nCurrChunk ].nSize > nFramesIn )
    {
	m_nBufferSize -= m_vBuffer[ m_nCurrChunk ].nSize - nFramesIn;
    }

    // copying audio from the input to the current chunk
    memcpy( m_vBuffer[ m_nCurrChunk ].pData, pBuffer, nFramesIn * 2 );
    m_vBuffer[ m_nCurrChunk ].nSize = nFramesIn;

    m_nCurrChunk++;
    // are we exceeding the current buffer chunk count?
    if( m_nCurrChunk > (int)m_vBuffer.size() - 1 )
    {
	// do we have to allocate more buffer space? (new chunk)
	// (more buffer space = more delay in time)
	if( m_nBufferSize > m_nDelayFramesCount )
	{
	    // so we don't have to allocate more space, jumping back to the first chunk
	    m_nCurrChunk = 0;
	}
    }

    sprintf( m_sDebugString, "compressor | m_nCurrChunk: %d m_nBufferSize: %d m_nDelayFramesCount: %d m_vBuffer.size(): %d m_fRatio: %f m_nPeak: %d\n", m_nCurrChunk, m_nBufferSize, m_nDelayFramesCount, m_vBuffer.size(), m_fRatio, m_nPeak );
    g_Log.log( CLOG_DEBUG_EXTREME, m_sDebugString );

    return m_pOut;
}
/*
void CCompressor::doCalcRMSVolume()
{
    m_dRMSVol = 0;

    for( int n = 0; n < m_nBufferSize; n++ )
    {
	short t = m_pBuffer[n];
	m_dRMSVol += t * t;
    }

    m_dRMSVol /= m_nBufferSize;
    pow( m_dRMSVol, 0.5f );
    m_dRMSVol = 20*log10( m_dRMSVol );

    m_nBufferPos = 0;
}*/
