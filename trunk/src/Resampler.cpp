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

#include "Resampler.h"
#include "Main.h"
#include "Log.h"

extern CLog g_Log;

CResampler::~CResampler()
{
    destroy();
}

void CResampler::init( double dRatio, int m_nChannels )
{
    m_pSRCIn = NULL;
    m_pSRCOut = NULL;
    m_pDataOut = NULL;
    m_nResampleBufSize = 0;
    m_dRatio = dRatio;

    int err = 0;
    m_pSRCState = src_new( SRC_SINC_BEST_QUALITY, m_nChannels, &err );
    if( m_pSRCState == NULL )
    {
        g_Log.Error( "can't initialize sample rate conversion\n" );
        m_bResamplerEnabled = false;
        return;
    }
    m_bResamplerEnabled = true;
}

void CResampler::destroy()
{
    m_pSRCState = src_delete( m_pSRCState );
    SAFE_DELETE_ARRAY( m_pSRCIn );
    SAFE_DELETE_ARRAY( m_pSRCOut );
    SAFE_DELETE_ARRAY( m_pDataOut );
}

short* CResampler::resample( short* pData, int nFramesNum, int& nOutFramesNum )
{
    if( !m_bResamplerEnabled )
    {
	nOutFramesNum = nFramesNum;
	return pData;
    }

    // checking that if buffer sizes are bigger than the incoming frames count
    if( m_nResampleBufSize < nFramesNum )
    {
	m_nResampleBufSize = nFramesNum;
	SAFE_DELETE_ARRAY( m_pSRCIn );
	m_pSRCIn = new float[ m_nResampleBufSize ];
	SAFE_DELETE_ARRAY( m_pSRCOut );
	m_pSRCOut = new float[ m_nResampleBufSize ];
	SAFE_DELETE_ARRAY( m_pDataOut );
	m_pDataOut = new short[ m_nResampleBufSize ];

	m_SRCData.data_in = m_pSRCIn;
	m_SRCData.data_out = m_pSRCOut;
	m_SRCData.output_frames = m_nResampleBufSize;
	m_SRCData.src_ratio = m_dRatio;
	m_SRCData.end_of_input = 0;
    }

    m_SRCData.input_frames = nFramesNum;

    src_short_to_float_array( pData, m_pSRCIn, nFramesNum );
    src_process( m_pSRCState, &m_SRCData );
    src_float_to_short_array( m_pSRCOut, m_pDataOut, m_SRCData.output_frames_gen );

    nOutFramesNum = m_SRCData.output_frames_gen;
    return m_pDataOut;
}
