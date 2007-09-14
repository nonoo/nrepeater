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

#ifndef __LOOP_H
#define __LOOP_H

#include "Compressor.h"
#include "Resampler.h"
#include "WavFile.h"
#include "DTMF.h"

class CLoop
{
public:
    CLoop();
    ~CLoop();

    void start();

    void setAlarm( int nMilliSecs );
    void clearAlarm();

    bool switchParrotMode();
    void checkDTMFSequence();

    void playWavFileBlocking( CWavFile& WavFile );
    void playWavFileNonBlocking( CWavFile& WavFile );

    void playBufferBlocking( short* pBuffer, int nBufferSize );
    void playBufferNonBlocking( short* pBuffer, int nBufferSize );

private:
    void parrotReceivingOver();


    struct timeval	m_tTime;
    int			m_nFDIn;
    fd_set		m_fsReads;
    int			m_nSelectRes;
    bool		m_bSquelchOff;

    CWavFile		m_RogerBeep;
    CWavFile		m_AckBeep;
    CWavFile		m_FailBeep;
    CCompressor		m_Compressor;
    CResampler		m_Resampler;

    CDTMF		m_DTMF;
    char*		m_pszDTMFDecoded;
    bool		m_bDTMFProcessingSuccess;

    // audio data from the sound card
    short*		m_pBuffer;
    int			m_nFramesRead;

    short*		m_pResampledData;
    short*		m_pCompOut;
    int			m_nCompressedFramesNum;
    int			m_nResampledFramesNum;

    int			m_nParrotBufferFree;
    int			m_nParrotBufferSize;
    short*		m_pParrotBuffer;
    int			m_nParrotBufferPos;
    bool		m_bParrotMode;

    short*		m_pWaveData;
    int			m_nWaveDataLength;
    int			m_nBufferPos;

// these variables have to be reached by onSIGALRM()
public:
    bool		m_bPlayingBeepStart;
    bool		m_bPlayingBeep;
    bool		m_bPlayRogerBeep;
    bool		m_bPlayAckBeep;
    bool		m_bPlayFailBeep;
};

#endif
