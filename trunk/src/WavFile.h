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

#ifndef __WAVFILE_H
#define __WAVFILE_H

#include <string>
#include <sndfile.h>

class CWavFile
{
public:
    CWavFile();
    ~CWavFile();

    void	openForWrite( std::string sFile, int nSampleRate, int nChannels, int nFormat );
    int		write( short* pData, int nFramesNum );
    bool	isOpened();
    void	loadToMemory( std::string sFile );
    bool	isLoaded();
    void	close();
    void	rewind();
    short*	play( int nBufferSize, int& nFramesRead );
    short*	getWaveData( int& nLength );
    int		getSampleRate();
    int		getChannelNum();
    void	setVolume( int nPercent );

private:
    SNDFILE*	m_pSNDFILE;
    SF_INFO	m_SFINFO;
    int		m_nSeek;
    short*	m_pWave;
};

#endif
