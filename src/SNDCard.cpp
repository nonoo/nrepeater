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

#include "Main.h"
#include "soundcard.h"
#include "SNDCard.h"
#include "Log.h"

#include <fcntl.h>
#include <sys/ioctl.h>

extern CLog g_Log;

CSNDCard::CSNDCard( string szDevName, int nMode, int nRate, int nChannels )
{
    m_szDevName = szDevName;
    m_pBuffer = NULL;
    m_nBufferSize = 0;
    m_nRate = nRate;
    m_nChannels = nChannels;
    m_nFormat = AFMT_S16_NE;
    m_nMode = nMode;
    m_nFDOut = -1;
    m_nFDIn = -1;
    m_bFirstTime = true; // for writing silence to the output for the first time

    int fd = -1;
    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    fd = open( szDevName.c_str() , O_WRONLY, 0 );
	    break;
	case SNDCARDMODE_IN:
	    fd = open( szDevName.c_str() , O_RDONLY, 0 );
	    break;
	default:
	    fd = open( szDevName.c_str() , O_RDWR, 0 );
    }

    if ( fd == -1 )
    {
	g_Log.log( CLOG_ERROR, "failed to open " + szDevName + "!\n" );
	exit( -1 );
    }

    int devcaps;
    if ( ioctl( fd, SNDCTL_DSP_GETCAPS, &devcaps ) == -1 )
    {
        g_Log.log( CLOG_ERROR, "SNDCTL_DSP_GETCAPS: " + szDevName + "\n" );
	exit( -1 );
    }

    int frag = 0x7fff000a; // Unlimited number of 1k fragments
    if( ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &frag ) == -1 )
    {
	g_Log.log( CLOG_ERROR, "SNDCTL_DSP_SETFRAGMENT: " + szDevName + "\n" );
	exit ( -1 );
    }

    if( m_nMode == SNDCARDMODE_DUPLEX )
    {
	if( !( devcaps & PCM_CAP_DUPLEX ) )
	{
	    g_Log.log( CLOG_ERROR, szDevName + " doesn't support one device based full duplex scheme. Please use the two device scheme.\n" );
	    exit( -1 );
	}

	if( ioctl( fd, SNDCTL_DSP_SETDUPLEX, NULL ) == -1 )
	{
    	    g_Log.log( CLOG_ERROR, "can't set full duplex mode on " + szDevName + "\n" );
	    exit( -1 );
	}
    }

    int tmp = m_nChannels;
    if( ioctl( fd, SNDCTL_DSP_CHANNELS, &tmp ) == -1 )
    {
	g_Log.log( CLOG_ERROR, "SNDCTL_DSP_CHANNELS: " + szDevName + "\n" );
	exit( -1 );
    }

    if( tmp != m_nChannels )
    {
	char etmp[100];
	sprintf( etmp, "can't open %d channels on %s\n", m_nChannels, szDevName.c_str() );
	g_Log.log( CLOG_ERROR, etmp );
	exit( -1 );
    }

    tmp = m_nFormat;
    if( ioctl( fd, SNDCTL_DSP_SETFMT, &tmp ) == -1 )
    {
	g_Log.log( CLOG_ERROR, "can't set audio format for " + szDevName + "\n" );
	exit( -1 );
    }

    if ( tmp != m_nFormat )
    {
	g_Log.log( CLOG_ERROR, szDevName + " doesn't support the given sample format\n" );
        exit( -1 );
    }

    tmp = m_nRate;
    if ( ioctl( fd, SNDCTL_DSP_SPEED, &tmp ) == -1 )
    {
	char etmp[100];
	sprintf( etmp, "can't set requested (%dhz) sample rate on %s\n", m_nRate, szDevName.c_str() );
        g_Log.log( CLOG_ERROR, etmp );
	exit( -1 );
    }

    if ( tmp != m_nRate )
    {
	char tmpstr[100];
	sprintf( tmpstr, "%s does not support requested sample rate (%dhz), switching to: %dhz\n", szDevName.c_str(), m_nRate, tmp );
        g_Log.log( CLOG_ERROR, tmpstr );

	m_nRate = tmp;
    }

    if ( ioctl( fd, SNDCTL_DSP_GETBLKSIZE, &m_nFragSize ) == -1 )
    {
        g_Log.log( CLOG_ERROR, "SNDCTL_DSP_GETBLKSIZE: " + szDevName + "\n" );
	exit( -1 );
    }

    m_nBufferSize = m_nFragSize * 2;
    m_pBuffer = new short[ m_nBufferSize ];
    memset( m_pBuffer, 0, m_nBufferSize );

    if ( (int)m_nFragSize > m_nBufferSize )
    {
	char etmp[100];
	sprintf( etmp, "too large fragment size (%d) for current buffer size (%d) for %s\n", m_nFragSize, m_nBufferSize, szDevName.c_str() );
        g_Log.log( CLOG_ERROR, etmp );
	exit( -1 );
    }

    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    m_nFDOut = fd;
	    break;
	case SNDCARDMODE_IN:
	    m_nFDIn = fd;
	    read( tmp ); // initializing sound card, if we don't do this, select() will timeout
	    break;
	default:
	    m_nFDOut = m_nFDIn = fd;
	    read( tmp ); // initializing sound card, if we don't do this, select() will timeout
	    break;
    }

    stop();

    char etmp[100];
    sprintf( etmp, "Initialized %s using fragment size %d, buffer size %d.\n", m_szDevName.c_str(), m_nFragSize, m_nBufferSize );
    g_Log.log( CLOG_DEBUG, etmp );
}

CSNDCard::~CSNDCard()
{
    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    close( m_nFDOut );
	    break;
	case SNDCARDMODE_IN:
	    close( m_nFDIn );
	    break;
	default:
	    close( m_nFDIn );
    }
    SAFE_DELETE_ARRAY( m_pBuffer );
}

// starts the device
void CSNDCard::start()
{
    int trig = 0;
    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    trig = PCM_ENABLE_OUTPUT;
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( CLOG_DEBUG, "SNDCTL_DSP_SETTRIGGER OUT 1" + m_szDevName + "\n" );
	    }
	    break;
	case SNDCARDMODE_IN:
	    trig = PCM_ENABLE_INPUT;
	    if( ioctl( m_nFDIn, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( CLOG_DEBUG, "SNDCTL_DSP_SETTRIGGER IN 1" + m_szDevName + "\n" );
	    }
	    break;
	default:
	    trig = PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( CLOG_DEBUG, "SNDCTL_DSP_SETTRIGGER INOUT 1" + m_szDevName + "\n" );
	    }
    }
}

// stops the device
void CSNDCard::stop()
{
    int trig = 0; // trigger off
    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( CLOG_DEBUG, "SNDCTL_DSP_SETTRIGGER OUT 0" + m_szDevName + "\n" );
	    }
	    break;
	case SNDCARDMODE_IN:
	    if( ioctl( m_nFDIn, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( CLOG_DEBUG, "SNDCTL_DSP_SETTRIGGER IN 0" + m_szDevName + "\n" );
	    }
	    break;
	default:
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( CLOG_DEBUG, "SNDCTL_DSP_SETTRIGGER INOUT 0" + m_szDevName + "\n" );
	    }
    }

    ioctl( m_nFDOut, SNDCTL_DSP_SILENCE, NULL);
    ioctl( m_nFDOut, SNDCTL_DSP_SKIP, NULL);
    ioctl( m_nFDOut, SNDCTL_DSP_HALT, NULL);
}

short* CSNDCard::read( int& nLength )
{
    struct audio_buf_info info;
    if( ioctl( m_nFDIn, SNDCTL_DSP_GETISPACE, &info ) == -1 )
    {
	g_Log.log( CLOG_ERROR, "GETISPACE " + m_szDevName + "\n" );
	exit( -1 );
    }

    int nReadFrames; // how much to read

    if( info.bytes > m_nBufferSize * 2 )
    {
	nReadFrames = m_nBufferSize;
    }
    else
    {
	nReadFrames = info.bytes / 2;
    }

    if( ( nLength = ::read( m_nFDIn, m_pBuffer, nReadFrames * 2 ) ) != nReadFrames * 2 )
    {
	g_Log.log( CLOG_ERROR, "error reading audio data from " + m_szDevName + "\n" );
	exit( -1 );
    }

    nLength /= 2;

    return m_pBuffer;
}

void CSNDCard::write( short* pBuffer, int nLength )
{
    if( m_bFirstTime)
    {
	short* pSilence = new short[ m_nFragSize ];
	memset( pSilence, 0, m_nFragSize * 2 );
	if( ::write( m_nFDOut, pSilence, m_nFragSize * 2 ) != (int)m_nFragSize * 2 )
	{
	    g_Log.log( CLOG_ERROR, "error writing audio data to " + m_szDevName + "\n" );
	    exit( -1 );
        }
	SAFE_DELETE_ARRAY( pSilence );
	m_bFirstTime = false;
    }

    // n * 2 because we read shorts
    if( ::write( m_nFDOut, pBuffer, nLength * 2 ) != (int)nLength * 2 )
    {
	g_Log.log( CLOG_ERROR, "error writing audio data to " + m_szDevName + "\n" );
	exit( -1 );
    }
}

int CSNDCard::getFDIn()
{
    return m_nFDIn;
}

int CSNDCard::getBufferSize()
{
    return m_nBufferSize;
}

int CSNDCard::getSampleRate()
{
    return m_nRate;
}

int CSNDCard::getChannelNum()
{
    return m_nChannels;
}
