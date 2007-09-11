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

#ifndef __DTMF_H
#define __DTMF_H

#include "Goertzel.h"

class CDTMF
{
public:
    void	init( int nSampleRate );
    void	process( short* pData, int nFramesNum );
    char*	finishDecoding();
    void	clearSequence();
    bool	isValidSequence( char* pszSequence );
    void	processSequence( char* pszSequence );

private:
    CGoertzel	m_Goertzel;
    int		m_nSampleRate;

    char	m_cDecoded;
    char	m_caDecodedChars[50];
    int		m_nDecodedCharsNum;
    char	m_cPrevDecoded;
    int		m_nSamplesProcessed;
    int		m_nPause;

    char*	m_pszSequence;
};

#endif
