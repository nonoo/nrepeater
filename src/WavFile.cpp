#include "WavFile.h"
#include "Log.h"
#include "Main.h"

extern CLog g_Log;

CWavFile::CWavFile( string sFile )
{
    memset( &m_SFINFO, 0, sizeof( m_SFINFO ) );
    if( ( m_pSNDFILE = sf_open( sFile.c_str(), SFM_READ, &m_SFINFO ) ) == NULL )
    {
	char errstr[100];
	sf_error_str( m_pSNDFILE, errstr, 100 );
	g_Log.Error( "can't open wav file " + sFile + ": " + errstr + "\n" );
	exit( -1 );
    }

    // loading wave data to memory
    m_pWave = new short[ m_SFINFO.frames ];
    if( sf_read_short( m_pSNDFILE, m_pWave, m_SFINFO.frames ) < m_SFINFO.frames )
    {
	char errstr[100];
	sf_error_str( m_pSNDFILE, errstr, 100 );
	g_Log.Error( "can't load wav file " + sFile + ": " + errstr + "\n" );
	exit( -1 );
    }

    init();
}

CWavFile::~CWavFile()
{
    sf_close( m_pSNDFILE );
    SAFE_DELETE_ARRAY( m_pWave );
}

// seeks to the beginning of the wave pointer
void CWavFile::init()
{
    m_nSeek = 0;
}

// nBufferSize: available sound buffer
// nFramesRead: count of frames read to the buffer
short* CWavFile::play( int nBufferSize, int& nFramesRead )
{
    if( m_nSeek >= m_SFINFO.frames )
    {
	return NULL;
    }

    if( nBufferSize + m_nSeek > m_SFINFO.frames )
    {
	nFramesRead = m_SFINFO.frames - m_nSeek;
    }
    else
    {
	nFramesRead = nBufferSize;
    }
    int nOldSeek = m_nSeek;
    m_nSeek += nFramesRead;

    return m_pWave + nOldSeek;
}

short* CWavFile::getWaveData( int& nLength )
{
    nLength = m_SFINFO.frames;
    return m_pWave;
}

int CWavFile::getSampleRate()
{
    return m_SFINFO.samplerate;
}

int CWavFile::getChannelNum()
{
    return m_SFINFO.channels;
}

void CWavFile::setVolume( int nPercent )
{
    for( int n=0; n < m_SFINFO.frames; n++ )
    {
	short t = m_pWave[n];
	m_pWave[n] = ( nPercent * t ) / 100;
    }
}
