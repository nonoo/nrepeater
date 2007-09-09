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

#ifndef __ARCHIVER_H
#define __ARCHIVER_H

#include "WavFile.h"
#include "SpeexCodec.h"
#include "OggFileOutStream.h"

#include <string>

class CArchiver
{
public:
    CArchiver();
    ~CArchiver();

    void init( int nSampleRate, int nChannels );
    void write( short* pData, int nFramesNum );
    void event( std::string sEvent );
    void event2( std::string sEvent );
    void maintain();

private:
    std::string	currDate();
    std::string getLogFileName();

    COggFileOutStream*	m_pOgg;
    CSpeexCodec		m_SpeexCodec;
    int			m_nSampleRate;
    int			m_nChannels;
    bool		m_bArchiverEnabled;
    std::string		m_szLogFileName;

    struct tm*		m_stLocalTime;
    time_t		m_stTime;
    int			m_nDay;

    FILE*		m_pEventFile;

    long		m_lArchivedSamples;
};

#endif
