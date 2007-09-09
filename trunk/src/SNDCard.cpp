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

#include <fcntl.h>
#include <sys/ioctl.h>
#include "soundcard.h"

#include "SNDCard.h"
#include "Log.h"
#include "Main.h"

extern CLog g_Log;

CSNDCard::CSNDCard( string sDevName, int nMode, int nRate, int nChannels )
{
    m_sDevName = sDevName;
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
	    fd = open( sDevName.c_str() , O_WRONLY, 0 );
	    break;
	case SNDCARDMODE_IN:
	    fd = open( sDevName.c_str() , O_RDONLY, 0 );
	    break;
	default:
	    fd = open( sDevName.c_str() , O_RDWR, 0 );
    }

    if ( fd == -1 )
    {
	g_Log.log( LOG_ERROR, "failed to open " + sDevName + "!\n" );
	exit( -1 );
    }

    int devcaps;
    if ( ioctl( fd, SNDCTL_DSP_GETCAPS, &devcaps ) == -1 )
    {
        g_Log.log( LOG_ERROR, "SNDCTL_DSP_GETCAPS: " + sDevName + "\n" );
	exit( -1 );
    }

    int frag = 0x7fff000a; // Unlimited number of 1k fragments
    if( ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &frag ) == -1 )
    {
	g_Log.log( LOG_ERROR, "SNDCTL_DSP_SETFRAGMENT: " + sDevName + "\n" );
	exit ( -1 );
    }

    if( m_nMode == SNDCARDMODE_DUPLEX )
    {
	if( !( devcaps & PCM_CAP_DUPLEX ) )
	{
	    g_Log.log( LOG_ERROR, sDevName + " doesn't support one device based full duplex scheme. Please use the two device scheme.\n" );
	    exit( -1 );
	}

	if( ioctl( fd, SNDCTL_DSP_SETDUPLEX, NULL ) == -1 )
	{
    	    g_Log.log( LOG_ERROR, "can't set full duplex mode on " + sDevName + "\n" );
	    exit( -1 );
	}
    }

    int tmp = m_nChannels;
    if( ioctl( fd, SNDCTL_DSP_CHANNELS, &tmp ) == -1 )
    {
	g_Log.log( LOG_ERROR, "SNDCTL_DSP_CHANNELS: " + sDevName + "\n" );
	exit( -1 );
    }

    if( tmp != m_nChannels )
    {
	char etmp[100];
	sprintf( etmp, "can't open %d channels on %s\n", m_nChannels, sDevName.c_str() );
	g_Log.log( LOG_ERROR, etmp );
	exit( -1 );
    }

    tmp = m_nFormat;
    if( ioctl( fd, SNDCTL_DSP_SETFMT, &tmp ) == -1 )
    {
	g_Log.log( LOG_ERROR, "can't set audio format for " + sDevName + "\n" );
	exit( -1 );
    }

    if ( tmp != m_nFormat )
    {
	g_Log.log( LOG_ERROR, sDevName + " doesn't support the given sample format\n" );
        exit( -1 );
    }

    tmp = m_nRate;
    if ( ioctl( fd, SNDCTL_DSP_SPEED, &tmp ) == -1 )
    {
	char etmp[100];
	sprintf( etmp, "can't set requested (%dhz) sample rate on %s\n", m_nRate, sDevName.c_str() );
        g_Log.log( LOG_ERROR, etmp );
	exit( -1 );
    }

    if ( tmp != m_nRate )
    {
	char tmpstr[100];
	sprintf( tmpstr, "%s does not support requested sample rate (%dhz), switching to: %dhz\n", sDevName.c_str(), m_nRate, tmp );
        g_Log.log( LOG_ERROR, tmpstr );

	m_nRate = tmp;
    }

    if ( ioctl( fd, SNDCTL_DSP_GETBLKSIZE, &m_nFragSize ) == -1 )
    {
        g_Log.log( LOG_ERROR, "SNDCTL_DSP_GETBLKSIZE: " + sDevName + "\n" );
	exit( -1 );
    }

    m_nBufferSize = m_nFragSize * 2;
    m_pBuffer = new short[ m_nBufferSize ];
    memset( m_pBuffer, 0, m_nBufferSize );

    if ( (int)m_nFragSize > m_nBufferSize )
    {
	char etmp[100];
	sprintf( etmp, "too large fragment size (%d) for current buffer size (%d) for %s\n", m_nFragSize, m_nBufferSize, sDevName.c_str() );
        g_Log.log( LOG_ERROR, etmp );
	exit( -1 );
    }

    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    m_nFDOut = fd;
	    break;
	case SNDCARDMODE_IN:
	    m_nFDIn = fd;
	    Read( tmp ); // initializing sound card, if we don't do this, select() will timeout
	    break;
	default:
	    m_nFDOut = m_nFDIn = fd;
	    Read( tmp ); // initializing sound card, if we don't do this, select() will timeout
	    break;
    }

    Stop();

    char etmp[100];
    sprintf( etmp, "Initialized %s using fragment size %d, buffer size %d.\n", m_sDevName.c_str(), m_nFragSize, m_nBufferSize );
    g_Log.log( LOG_DEBUG, etmp );
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
void CSNDCard::Start()
{
    int trig = 0;
    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    trig = PCM_ENABLE_OUTPUT;
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( LOG_DEBUG, "SNDCTL_DSP_SETTRIGGER OUT 1" + m_sDevName + "\n" );
	    }
	    break;
	case SNDCARDMODE_IN:
	    trig = PCM_ENABLE_INPUT;
	    if( ioctl( m_nFDIn, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( LOG_DEBUG, "SNDCTL_DSP_SETTRIGGER IN 1" + m_sDevName + "\n" );
	    }
	    break;
	default:
	    trig = PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( LOG_DEBUG, "SNDCTL_DSP_SETTRIGGER INOUT 1" + m_sDevName + "\n" );
	    }
    }
}

// stops the device
void CSNDCard::Stop()
{
    int trig = 0; // trigger off
    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( LOG_DEBUG, "SNDCTL_DSP_SETTRIGGER OUT 0" + m_sDevName + "\n" );
	    }
	    break;
	case SNDCARDMODE_IN:
	    if( ioctl( m_nFDIn, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( LOG_DEBUG, "SNDCTL_DSP_SETTRIGGER IN 0" + m_sDevName + "\n" );
	    }
	    break;
	default:
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.log( LOG_DEBUG, "SNDCTL_DSP_SETTRIGGER INOUT 0" + m_sDevName + "\n" );
	    }
    }

    ioctl( m_nFDOut, SNDCTL_DSP_SILENCE, NULL);
    ioctl( m_nFDOut, SNDCTL_DSP_SKIP, NULL);
    ioctl( m_nFDOut, SNDCTL_DSP_HALT, NULL);
}

short* CSNDCard::Read( int& nLength )
{
    struct audio_buf_info info;
    if( ioctl( m_nFDIn, SNDCTL_DSP_GETISPACE, &info ) == -1 )
    {
	g_Log.log( LOG_ERROR, "GETISPACE " + m_sDevName + "\n" );
	exit( -1 );
    }

    int n = info.bytes; // how much to read

    if( read( m_nFDIn, m_pBuffer, n * 2 ) != n * 2 ) // n * 2 because we read shorts
    {
	g_Log.log( LOG_ERROR, "error reading audio data from " + m_sDevName + "\n" );
	exit( -1 );
    }

    nLength = n;

    return m_pBuffer;
}

void CSNDCard::Write( short* pBuffer, int nLength )
{
    if( m_bFirstTime)
    {
	short* pSilence = new short[ m_nFragSize ];
	memset( pSilence, 0, m_nFragSize * 2 );
	if( write( m_nFDOut, pSilence, m_nFragSize * 2 ) != (int)m_nFragSize * 2 )
	{
	    g_Log.log( LOG_ERROR, "error writing audio data to " + m_sDevName + "\n" );
	    exit( -1 );
        }
	SAFE_DELETE_ARRAY( pSilence );
	m_bFirstTime = false;
    }

    // n * 2 because we read shorts
    if( write( m_nFDOut, pBuffer, nLength * 2 ) != (int)nLength * 2 )
    {
	g_Log.log( LOG_ERROR, "error writing audio data to " + m_sDevName + "\n" );
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
