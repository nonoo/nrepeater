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

#ifndef __GOERTZEL_H
#define __GOERTZEL_H

#include <string>

#define MAX_BINS	8
#define GOERTZEL_N	92
#define MAX_DECODED	10

class CGoertzel
{
public:
    void	init( int nSampleRate );
    char*	process( short* pData, int nFramesNum );

private:
    void	calcCoeffs();
    void	postTesting();

    int		m_nSampleRate;
    char	m_caDTMFASCII[4][4];
    double	m_daFreqs[ MAX_BINS ];
    double	m_daCoeffs[ MAX_BINS ];
    double	m_dQ0;
    double	m_daQ1[ MAX_BINS ];
    double	m_daQ2[ MAX_BINS ];
    double	m_daR[ MAX_BINS ];
    int		m_nSampleCount;

    int		i,j;

    int		m_nRow;
    double	m_dMaxVal;
    double	m_dT;
    int		m_nCol;
    bool	m_bSeeDigit;
    int		m_nMaxIndex;
    int		m_nPeakCount;

    char	m_caDecoded[ MAX_DECODED ];
    int		m_nDecodedNum;
};

#endif
