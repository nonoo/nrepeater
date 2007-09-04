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

#ifndef __COMPRESSOR_H
#define __COMPRESSOR_H

#include <vector>

class CCompressor
{
public:
    short* Process( short* pBuffer, int nFramesIn, int& nFramesOut );
    void doCalcRMSVolume();

    CCompressor( int nSNDCardRate, int nSNDCardBufferSize );
    ~CCompressor();

private:
    void DoCompress();

    typedef struct
    {
	short*	pData;
	int	nSize;
    } tBufferChunk;

    std::vector< tBufferChunk >			m_vBuffer;

    int		m_nCurrChunk;
    int		m_nBufferSize;
    int		m_nDelayFramesCount;
    short*	m_pOut;
    int		m_nSampleRate;

/*    int		m_nBufferPos;
    double	m_dRMSVol;
    float	m_fThreshold;
    float	m_fOrigRatio, m_fRatio;
    int		m_nMakeUpGain;*/
};

#endif
