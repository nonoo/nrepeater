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

#include "Archiver.h"
#include "Main.h"
#include "SettingsFile.h"
#include "Log.h"

#include <sndfile.h>
#include <time.h>

#define SPEEX_SAMPLERATE 8000

using namespace std;

extern CLog		g_Log;
extern CSettingsFile	g_MainConfig;

FILE*f=fopen("a.raw","w");
CArchiver::~CArchiver()
{
    m_pSRCState = src_delete( m_pSRCState );
    SAFE_DELETE( m_pOgg );
    SAFE_DELETE_ARRAY( m_pSRCIn );
    SAFE_DELETE_ARRAY( m_pSRCOut );
    SAFE_DELETE_ARRAY( m_pDataOut );
    fclose(f);
}

void CArchiver::init( int nSampleRate, int nChannels )
{
    m_nSampleRate = nSampleRate;
    m_nChannels = nChannels;
    m_pSRCIn = NULL;
    m_pSRCOut = NULL;
    m_pSRCState = NULL;
    m_pDataOut = NULL;
    m_nResampleBufSize = 0;
    m_bArchiverEnabled = true;
}

void CArchiver::write( short* pData, int nFramesNum )
{
    if( !m_bArchiverEnabled )
    {
	return;
    }

    if( m_pOgg == NULL )
    {
	m_SpeexCodec.destroy();
	m_pOgg = new COggFileOutStream( 0 );
	m_pOgg->open( string( g_MainConfig.Get( "archiver", "dir", "./" ) + g_MainConfig.Get( "archiver", "prefix", "log-" ) + currDate() + ".spx" ).c_str() );
	m_SpeexCodec.initEncode( m_pOgg, SPEEX_SAMPLERATE, m_nChannels, g_MainConfig.GetInt( "archiver", "bitrate", 10000 ) );
    }

    if( m_nSampleRate == SPEEX_SAMPLERATE )
    {
	m_SpeexCodec.encode( pData, nFramesNum );
    }
    else
    {
	// resampling
	if( m_nResampleBufSize < nFramesNum )
	{
	    cout << m_nResampleBufSize << endl;
	    int err;
	    m_pSRCState = src_delete( m_pSRCState );
	    m_pSRCState = src_new( SRC_SINC_BEST_QUALITY, m_nChannels, &err );
	    if( m_pSRCState == NULL )
	    {
		g_Log.Error( "can't initialize sample rate conversion for archiver!\n" );
		m_bArchiverEnabled = false;
		return;
	    }

	    SAFE_DELETE_ARRAY( m_pSRCIn );
	    m_pSRCIn = new float[ nFramesNum ];
	    memset(m_pSRCIn,0,nFramesNum);
	    SAFE_DELETE_ARRAY( m_pSRCOut );
	    m_pSRCOut = new float[ nFramesNum ];
	    memset(m_pSRCOut,0,nFramesNum);
	    SAFE_DELETE_ARRAY( m_pDataOut );
	    m_pDataOut = new short[ nFramesNum ];
	    memset(m_pDataOut,0,nFramesNum);

	    m_nResampleBufSize = nFramesNum;

	    m_SRCData.data_in = m_pSRCIn;
	    m_SRCData.data_out = m_pSRCOut;
	    m_SRCData.input_frames = nFramesNum;
	    m_SRCData.output_frames = nFramesNum;
	    m_SRCData.src_ratio = (double)SPEEX_SAMPLERATE / m_nSampleRate;
	    m_SRCData.end_of_input = 0;
	}

        //src_reset( m_pSRCState );

	src_short_to_float_array( pData, m_pSRCIn, nFramesNum );
	src_process( m_pSRCState, &m_SRCData );
	src_float_to_short_array( m_pSRCOut, m_pDataOut, m_SRCData.output_frames_gen );

	m_SpeexCodec.encode( m_pDataOut, m_SRCData.output_frames_gen );
	fwrite(m_pDataOut,m_SRCData.output_frames_gen*2,1,f);
    }
}

// returns current time in ymd format
string CArchiver::currDate()
{
    char tmp[50];
    time_t t = time( NULL );
    struct tm *tmpt = localtime( &t );
    strftime( tmp, 50, "%y%m%d", tmpt );
    return tmp;
}
