#ifndef __WAVFILE_H
#define __WAVFILE_H

#include <string>
#include <sndfile.h>

using namespace std;

class CWavFile
{
public:
    CWavFile( string sFile );
    ~CWavFile();

    void	init();
    short*	play( int nBufferSize, int& nFramesRead );
    short*	getWaveData( int& nLength );
    int		getSampleRate();
    int		getChannelNum();

private:
    SNDFILE*	m_pSNDFILE;
    SF_INFO	m_SFINFO;
    int		m_nSeek;
    short*	m_pWave;
};

#endif
