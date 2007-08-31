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

    void init();
    int play( short* pBuffer, int nSize );

private:
    SNDFILE* m_pSNDFILE;
    SF_INFO m_SFINFO;
};

#endif
