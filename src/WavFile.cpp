#include "WavFile.h"
#include "Log.h"

extern CLog g_Log;

CWavFile::CWavFile( string sFile )
{
    memset( &m_SFINFO, 0, sizeof( m_SFINFO ) );
    if( ( m_pSNDFILE = sf_open( sFile.c_str(), SFM_READ, &m_SFINFO ) ) == NULL )
    {
	char errstr[100];
	sf_error_str( m_pSNDFILE, errstr, 100 );
	g_Log.Error( "can't open roger beep file " + sFile + ": " + errstr + "\n" );
	exit( -1 );
    }
}

CWavFile::~CWavFile()
{
    sf_close( m_pSNDFILE );
}

// seeks to the beginning of the file
void CWavFile::init()
{
    sf_seek( m_pSNDFILE, 0, SEEK_SET );
}

int CWavFile::play( short* pBuffer, int nSize )
{
    return sf_read_short( m_pSNDFILE, pBuffer, nSize );
}
