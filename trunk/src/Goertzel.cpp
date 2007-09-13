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

// algorithm taken from http://en.wikipedia.org/wiki/Goertzel_algorithm
// more info: http://www.embedded.com/story/OEG20020819S0057

#include "Main.h"
#include "Goertzel.h"

#include <math.h>

using namespace std;

void CGoertzel::init( int nSampleRate )
{
    m_nSampleRate = nSampleRate;
    m_nSampleCount = 0;

    m_daFreqs[0] = 697;
    m_daFreqs[1] = 770;
    m_daFreqs[2] = 852;
    m_daFreqs[3] = 941;
    m_daFreqs[4] = 1209;
    m_daFreqs[5] = 1336;
    m_daFreqs[6] = 1477;
    m_daFreqs[7] = 1633;

    // 1 2 3 A
    // 4 5 6 B
    // 7 8 9 C
    // * 0 # D
    m_caDTMFASCII[0][0] = '1';
    m_caDTMFASCII[1][0] = '4';
    m_caDTMFASCII[2][0] = '7';
    m_caDTMFASCII[3][0] = '*';
    m_caDTMFASCII[0][1] = '2';
    m_caDTMFASCII[1][1] = '5';
    m_caDTMFASCII[2][1] = '8';
    m_caDTMFASCII[3][1] = '0';
    m_caDTMFASCII[0][2] = '3';
    m_caDTMFASCII[1][2] = '6';
    m_caDTMFASCII[2][2] = '9';
    m_caDTMFASCII[3][2] = '#';
    m_caDTMFASCII[0][3] = 'A';
    m_caDTMFASCII[1][3] = 'B';
    m_caDTMFASCII[2][3] = 'C';
    m_caDTMFASCII[3][3] = 'D';

    for( i = 0; i < MAX_BINS; i++ )
    {
	m_dQ0 = m_daCoeffs[i] = m_daQ1[i] = m_daQ2[i] = m_daR[i] = 0;
    }

    calcCoeffs();
}

char CGoertzel::process( short* pData, int nFramesNum )
{
    m_cDecoded = 0;

    for( j = 0; j < nFramesNum; j++ )
    {
	m_nSampleCount++;
	for( i = 0; i < MAX_BINS; i++ )
	{
	    m_dQ0 = m_daCoeffs[i] * m_daQ1[i] - m_daQ2[i] + pData[j];
	    m_daQ2[i] = m_daQ1[i];
	    m_daQ1[i] = m_dQ0;
	}

	if( m_nSampleCount == GOERTZEL_N )
	{
	    for( i = 0; i < MAX_BINS; i++ )
	    {
		m_daR[i] = ( m_daQ1[i] * m_daQ1[i] ) + ( m_daQ2[i] * m_daQ2[i] ) - ( m_daCoeffs[i] * m_daQ1[i] * m_daQ2[i] );
		m_daQ1[i] = 0.0;
		m_daQ2[i] = 0.0;
	    }
	    postTesting();
	    m_nSampleCount = 0;
	}
    }

    return m_cDecoded;
}

// coef = 2.0 * cos( ( 2.0 * PI * k ) / (float)GOERTZEL_N );
// where k = (int) ( 0.5 + ( (float)GOERTZEL_N * target_freq ) / SAMPLING_RATE) );
// more simply: coef = 2.0 * cos( (2.0 * PI * target_freq) / SAMPLING_RATE );
void CGoertzel::calcCoeffs()
{
    for( int n = 0; n < MAX_BINS; n++)
    {
	m_daCoeffs[n] = 2.0 * cos( 2.0 * 3.141592654 * m_daFreqs[n] / m_nSampleRate );
    }
}

void CGoertzel::postTesting()
{
    // find the largest in the row group
    m_nRow = 0;
    m_dMaxVal = 0;
    for( i = 0; i < 4; i++ )
    {
	if( m_daR[i] > m_dMaxVal )
	{
	    m_dMaxVal = m_daR[i];
	    m_nRow = i;
        }
    }

    // find the largest in the column group
    m_nCol = 4;
    m_dMaxVal = 0.0;
    for( i = 4; i < 8; i++ )
    {
	if( m_daR[i] > m_dMaxVal )
	{
	    m_dMaxVal = m_daR[i];
	    m_nCol = i;
	}
    }

    if( m_daR[ m_nRow ] < 4.0e5 ) // 2.0e5 ... 1.0e8 no change
    {
	// energy not high enough
    }
    else if( m_daR[ m_nCol ] < 4.0e5 )
    {
        // energy not high enough
    }
    else
    {
	m_bSeeDigit = true;

	// twist check
	// CEPT => twist < 6dB
	// AT&T => forward twist < 4dB and reverse twist < 8dB
	//  -ndB < 10 log10( v1 / v2 ), where v1 < v2
	//  -4dB < 10 log10( v1 / v2 )
	//  -0.4 < log10( v1 / v2 )
	// 0.398 < v1 / v2
	// 0.398 * v2 < v1

	if( m_daR[ m_nCol ] > m_daR[ m_nRow ] )
	{
	    // normal twist
	    m_nMaxIndex = m_nCol;
	    if ( m_daR[ m_nRow ] < ( m_daR[ m_nCol ] * 0.398 ) ) // twist > 4dB, error
    	    m_bSeeDigit = false;
	}
	else
	{
	    // reverse twist
	    m_nMaxIndex = m_nRow;
	    if ( m_daR[ m_nCol ] < ( m_daR[ m_nRow ] * 0.158 ) ) // twist > 8db, error
	    m_bSeeDigit = false;
	}

	// signal to noise test
	// AT&T states that the noise must be 16dB down from the signal
	// here we count the number of signals above the threshold and
	// there ought to be only two.

	if( m_daR[ m_nMaxIndex ] > 1.0e9 )
	{
	    m_dT = m_daR[ m_nMaxIndex ] * 0.158;
	}
	else
	{
	    m_dT = m_daR[ m_nMaxIndex ] * 0.010;
	}

	m_nPeakCount = 0;
	for( i = 0; i < 8; i++ )
	{
	    if( m_daR[i] > m_dT )
	    {
	        m_nPeakCount++;
	    }
	}
    	if( m_nPeakCount > 2 )
	{
	    m_bSeeDigit = false;
	}
	if( m_bSeeDigit )
	{
	    if( ( m_cDecoded != 0 ) && ( m_caDTMFASCII[ m_nRow ][ m_nCol - 4 ] != m_cDecoded ) )
	    {
		m_cDecoded = 0;
	    }
	    else
	    {
		m_cDecoded = m_caDTMFASCII[ m_nRow ][ m_nCol - 4 ];
	    }
	}
    }
}
