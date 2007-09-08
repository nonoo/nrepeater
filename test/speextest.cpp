#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/soundcard.h>
#include <speex/speex.h>
#include <speex/speex_preprocess.h>
#include <iostream>
using namespace std;

#define SAFE_DELETE(p)       { delete (p); (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p); (p)=NULL; }

int fd_in;
FILE* fd_out;
int sample_rate = 8000;
int m_nFrameSize;
void* m_pState;
SpeexBits m_spxBits;
SpeexPreprocessState* m_pPreProcState;
char* m_pOut;
short* m_pBuf;
int m_nBufSize;
int m_nBufStoredFramesNum ;

static int open_audio_device (char *name, int mode)
{
    int tmp, fd;
  
    if ((fd = open (name, mode, 0)) == -1)
    {
	    perror (name);
	    exit (-1);
    }

    tmp = AFMT_S16_NE;		/* Native 16 bits */
    if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
        perror ("SNDCTL_DSP_SETFMT");
	exit (-1);
    }

    if (tmp != AFMT_S16_NE)
    {
	fprintf (stderr, "The device doesn't support the 16 bit sample format.\n");
	exit (-1);
    }

    tmp = 1;
    if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
	perror ("SNDCTL_DSP_CHANNELS");
	exit (-1);
    }

    if (tmp != 1)
    {
	fprintf (stderr, "The device doesn't support mono mode.\n");
	exit (-1);
    }

    if (ioctl (fd, SNDCTL_DSP_SPEED, &sample_rate) == -1)
    {
	perror ("SNDCTL_DSP_SPEED");
	exit (-1);
    }
    return fd;
}
	  
void process_input (void)
{
    short buffer[1024];

    int l, i, level;

    if ((l = read (fd_in, buffer, sizeof (buffer))) == -1)
    {
	perror ("Audio read");
	exit (-1);		/* Or return an error code */
    }
    l = l / 2;
    level = 0;

    int nFramesNum = l;
    if( m_nBufSize < nFramesNum + m_nBufStoredFramesNum )
    {
	short* pTmp = NULL;
	if( m_nBufStoredFramesNum > 0 )
	{
	    // we have some data in the buffer
	    pTmp = new short[ m_nBufStoredFramesNum ];
	    memcpy( pTmp, m_pBuf, m_nBufStoredFramesNum * 2 );
	}

	// reallocating buffer
	SAFE_DELETE_ARRAY( m_pBuf );
	m_nBufSize = nFramesNum + m_nBufStoredFramesNum;
	m_pBuf = new short[ m_nBufSize ];

	if( m_nBufStoredFramesNum > 0 )
	{
	    // restoring previous data to the new buffer
	    memcpy( m_pBuf, pTmp, m_nBufStoredFramesNum * 2 );
	    SAFE_DELETE_ARRAY( pTmp );
	}
    }

    // adding incoming data to the buffer
    memcpy( m_pBuf + m_nBufStoredFramesNum, buffer, nFramesNum * 2 );
    m_nBufStoredFramesNum += nFramesNum;

    int nDataPos = 0;

    while( nDataPos + 160 <= m_nBufStoredFramesNum )
    {
	short* pTmp = new short[ 3200 ];
	memcpy( pTmp, m_pBuf + nDataPos, 160 * 2 );
	//speex_preprocess( m_pPreProcState, pTmp, NULL);
	speex_bits_reset( &m_spxBits );
	speex_encode_int( m_pState, pTmp, &m_spxBits );
    //fwrite( pTmp, 1, m_nFrameSize*2, fd_out );
	SAFE_DELETE_ARRAY( pTmp );
	nDataPos += m_nFrameSize;

	unsigned int nBytesOut = speex_bits_write( &m_spxBits, m_pOut, m_nFrameSize * 2 );

	/*m_Op.packet = (unsigned char*)m_pOut;
	m_Op.bytes = nBytesOut;
	m_Op.b_o_s = 0;
	m_Op.e_o_s = 0;
	m_Op.granulepos = ++m_lGranulePos;

	int tmp;
	speex_encoder_ctl( m_pState, SPEEX_GET_BITRATE, &tmp );
	cout << "spx bitrate: " << tmp << endl;

	m_pOgg->feedPacket( &m_Op, false );*/

    }

    cout << "incoming framesnum: " << nFramesNum << " processed: " << nDataPos << " remaining frames: " << m_nBufStoredFramesNum-nDataPos << " in buffer: " << m_nBufStoredFramesNum << endl;

    if( m_nBufStoredFramesNum - nDataPos > 0 )
    {
	// we still have unprocessed data in the buffer, storing it
	short* pTmp = new short[ m_nBufStoredFramesNum - nDataPos ];
	memcpy( pTmp, m_pBuf + nDataPos, ( m_nBufStoredFramesNum - nDataPos ) * 2 );
	memcpy( m_pBuf, pTmp, ( m_nBufStoredFramesNum - nDataPos ) * 2 );
	SAFE_DELETE_ARRAY( pTmp );
    }
    m_nBufStoredFramesNum = m_nBufStoredFramesNum - nDataPos;


    for (i = 0; i < l; i++)
    {
	int v = buffer[i];

	if (v < 0)
	v = -v;			/* abs */

	if (v > level)
	level = v;
    }
    level = (level + 1) / 1024;

    for (i = 0; i < level; i++)
	printf ("*");
    for (i = level; i < 32; i++)
	printf (".");
    printf ("\r");
    fflush (stdout);
}
		  
int main (int argc, char *argv[])
{
    char *name_in = "/dev/dsp";

    if (argc > 1)
	name_in = argv[1];
    fd_in = open_audio_device (name_in, O_RDONLY);

    fd_out = fopen( "out.raw", "w" );

    m_pState = speex_encoder_init( &speex_nb_mode );

    // setting encoder options
    int tmp = 10000;
    speex_encoder_ctl( m_pState, SPEEX_SET_ABR, &tmp);
    speex_encoder_ctl( m_pState, SPEEX_SET_BITRATE, &tmp);
    tmp = 1;
    speex_encoder_ctl( m_pState, SPEEX_SET_VBR, &tmp);
    speex_encoder_ctl( m_pState, SPEEX_SET_DTX, &tmp);
    //tmp = 10;
    //speex_encoder_ctl( m_pState, SPEEX_SET_VBR_QUALITY, &tmp);
    tmp = 10;
    speex_encoder_ctl( m_pState, SPEEX_SET_COMPLEXITY, &tmp);

    speex_encoder_ctl( m_pState, SPEEX_GET_FRAME_SIZE, &m_nFrameSize);

    tmp = 8000;
    speex_encoder_ctl( m_pState, SPEEX_SET_SAMPLING_RATE, &tmp );
    speex_encoder_ctl( m_pState, SPEEX_GET_SAMPLING_RATE, &tmp );

    m_pPreProcState = speex_preprocess_state_init( m_nFrameSize, tmp );

    // setting preprocessor options
    tmp = 1;
    speex_preprocess_ctl( m_pPreProcState, SPEEX_PREPROCESS_SET_DENOISE, &tmp );
    speex_preprocess_ctl( m_pPreProcState, SPEEX_PREPROCESS_SET_AGC, &tmp );
    speex_preprocess_ctl( m_pPreProcState, SPEEX_PREPROCESS_SET_VAD, &tmp );

    speex_bits_init( &m_spxBits );

    m_pOut = new char[ m_nFrameSize * 4 ];
    m_pBuf = NULL;
    m_nBufSize = 0;
    m_nBufStoredFramesNum = 0;

    while (1)
	process_input ();

    fclose( fd_out );

    speex_bits_destroy( &m_spxBits );
    if( m_pState != NULL )
    {
	speex_encoder_destroy( m_pState ); 
    }
    if( m_pPreProcState != NULL )
    {
	speex_preprocess_state_destroy( m_pPreProcState );
    }

    SAFE_DELETE_ARRAY( m_pOut );
    SAFE_DELETE_ARRAY( m_pBuf );

    exit (0);
}
