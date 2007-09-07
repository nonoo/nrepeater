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

#ifndef __RESAMPLER_H
#define __RESAMPLER_H

#include <samplerate.h>

class CResampler
{
public:
    ~CResampler();

    void init( double dRatio, int nChannels ); // nRatio: newsamplerate / oldsamplerate
    void destroy();

    short* resample( short* pData, int nFramesNum, int& nFramesOut );

private:
    float*		m_pSRCIn;
    float*		m_pSRCOut;
    short*		m_pDataOut;
    int			m_nResampleBufSize;
    bool		m_bResamplerEnabled;
    double		m_dRatio;

    SRC_STATE*		m_pSRCState;
    SRC_DATA		m_SRCData;
};

#endif
