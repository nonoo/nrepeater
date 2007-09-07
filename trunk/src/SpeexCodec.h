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

#ifndef __SPEEXCODEC_H
#define __SPEEXCODEC_H

#include <speex/speex.h>
#include <speex/speex_preprocess.h>
#include <ogg/ogg.h>

#include "OggOutStream.h"

class CSpeexCodec
{
public:
    CSpeexCodec();
    ~CSpeexCodec();

    void	initEncode( COggOutStream* pOGG, int nSampleRate, int nChannels, int nBitrate );
    void	destroy();
    void	encode( short* pData, int nFramesNum  );

private:
    void	generateHeader();
    void	generateComment();

    int				m_nSampleRate;
    int				m_nChannels;
    SpeexBits			m_spxBits;
    void*			m_pState;
    int				m_nFrameSize;
    char*			m_pOut;
    SpeexPreprocessState*	m_pPreProcState;
    long			m_lSerialNo;
    ogg_packet			m_Op;
    long			m_lGranulePos;
    COggOutStream*		m_pOgg;
    bool			m_bDecoder;

    short*			m_pBuf;
    int				m_nBufSize;
    int				m_nBufStoredFramesNum;
};

#endif
