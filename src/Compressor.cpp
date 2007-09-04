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

#include "Compressor.h"
#include "Main.h"
#include "SettingsFile.h"

extern CSettingsFile g_MainConfig;

CCompressor::CCompressor( int nSNDCardRate, int nSNDCardBufferSize )
{
    m_nSampleRate = nSNDCardRate;

    m_nDelayFramesCount = (int)( ( (float)g_MainConfig.GetInt( "compressor", "predelay", 1000 ) / 1000 ) * m_nSampleRate );
    m_nCurrChunk = 0;
    m_pOut = new short[ nSNDCardBufferSize ];
    m_nBufferSize = 0;
}

CCompressor::~CCompressor()
{
    SAFE_DELETE_ARRAY( m_pOut );

    for( vector< tBufferChunk >::iterator it = m_vBuffer.begin(); it != m_vBuffer.end(); it++ )
    {
	SAFE_DELETE_ARRAY( (*it).pData );
    }
}

short* CCompressor::Process( short* pBuffer, int nFramesIn, int& nFramesOut )
{
/*    if( m_nBufferPos + nBufferSize > m_nBufferSize )
    {
	memcpy( m_pBuffer + m_nBufferPos, pBuffer, m_nBufferSize - m_nBufferPos );
	doCalcRMSVolume();

	// volume level is above the threshold
	//cout << m_dRMSVol - 120 << endl;
	if( m_dRMSVol - 120 > m_fThreshold )
	{
	    m_nRatio = 10;
	    cout << "comp on" << endl;
	}
	else
	{
	    m_nRatio = 1;
	    cout << "comp off" << endl;
	}
    }
    else
    {
	memcpy( m_pBuffer + m_nBufferPos, pBuffer, nBufferSize );
	m_nBufferPos += nBufferSize;
    }*/

    // data comes in variable-sized chunks from the sound card.
    // we put each incoming chunk into the m_vBuffer vector and
    // add the incoming frame size to the m_nBufferSize.
    // if we got the number of samples needed to delay (m_vBuffer
    // is full == m_nBufferSize is at least the number of samples
    // we need to delay), we jump to the beginning of the vector.

    // do we have a chunk already allocated at the given slot?
    try
    {
	m_vBuffer.at( m_nCurrChunk );
    }
    catch ( exception e )
    {
	// creating a new chunk
	tBufferChunk bcTmp;
	bcTmp.pData = new short[ nFramesIn ];
	memset( bcTmp.pData, 0, nFramesIn * 2 );
	bcTmp.nSize = nFramesIn;
	m_nBufferSize += bcTmp.nSize;
	m_vBuffer.push_back( bcTmp );
    }

    // copying the audio from the current chunk to the output buffer
    memcpy( m_pOut, m_vBuffer[ m_nCurrChunk ].pData, m_vBuffer[ m_nCurrChunk ].nSize * 2 );
    nFramesOut = m_vBuffer[ m_nCurrChunk ].nSize;

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
	// do we have to allocate more buffer space?
	// (more buffer space = more delay in time)
	if( m_nBufferSize > m_nDelayFramesCount )
	{
	    // so we don't have to allocate more space, jumping back to the first chunk
	    m_nCurrChunk = 0;
	}
    }
    //cout << "m_nCurrChunk: " << m_nCurrChunk << " m_nBufferSize: " << m_nBufferSize << " m_nDelayFramesCount: " << m_nDelayFramesCount << " m_vBuffer.size(): " << m_vBuffer.size() << endl;

    return m_pOut;

/*    for( int i = 0; i < nBufferSize; i++ )
    {
	if( pBuffer[i] > 1500 )
	{
	    m_fRatio = m_fOrigRatio;
	}
	else
	{
	    if( m_fRatio > 1 )
	    {
		m_fRatio -= 0.0002;
	    }
	    else
	    {
		m_fRatio = 1;
	    }
	}

	// applying compression
	pBuffer[i] = (short)( pBuffer[i] * ( 1 / m_fRatio ) );

	// applying makeup gain
	pBuffer[i] = (short)( pBuffer[i] * pow( 10, (float)m_nMakeUpGain / 10 ) );
    }
	    cout << m_fRatio << endl;*/
}

void CCompressor::doCalcRMSVolume()
{
/*    m_dRMSVol = 0;

    for( int n = 0; n < m_nBufferSize; n++ )
    {
	short t = m_pBuffer[n];
	m_dRMSVol += t * t;
    }

    m_dRMSVol /= m_nBufferSize;
    pow( m_dRMSVol, 0.5f );
    m_dRMSVol = 20*log10( m_dRMSVol );

    m_nBufferPos = 0;*/
}
