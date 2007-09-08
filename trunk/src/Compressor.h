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

#define COMPSTATE_OFF		0
#define COMPSTATE_COMPRESSING	1
#define COMPSTATE_ATTACK	2
#define COMPSTATE_RELEASE	4

class CCompressor
{
public:
    ~CCompressor();

    void	init( int nSNDCardRate, int nSNDCardBufferSize );
    void	destroy();

    short*	process( short* pBuffer, int nFramesIn, int& nFramesOut );
    void	flush();

private:
    int		doCompress();
    void	calcRatio();

    typedef struct
    {
	short*	pData;
	int	nSize;
    } tBufferChunk;
    std::vector< tBufferChunk > m_vBuffer;

    bool	m_bCompressorEnabled;

    int		m_nCurrChunk;
    int		m_nBufferSize;
    int		m_nDelayFramesCount;
    short*	m_pOut;
    int		m_nOutSize;
    int		m_nSampleRate;
    int		m_nOrigRatio;
    float	m_fRatio;
    int		m_nMakeUpGain;
    int		m_nThreshold;
    int		m_sState;

    int		m_nAttackFramesCount;
    int		m_nCurrAttackFrameCount;
    int		m_nReleaseFramesCount;
    int		m_nCurrReleaseFrameCount;
    int		m_nHoldFramesCount;
    int		m_nCurrHoldFrameCount;

    int		m_nPeak;
    char	m_sDebugString[500];
};

#endif
