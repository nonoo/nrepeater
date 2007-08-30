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

CSNDCard::CSNDCard( string sDevName, int nMode )
{
    m_sDevName = sDevName;
    m_pBuffer = NULL;
    m_nBufferSize = 32*1024;
    m_nRate = 44100;
    m_nChannels = 1;
    m_nFormat = AFMT_S16_NE;
    m_nMode = nMode;
    m_nFDOut = -1;
    m_nFDIn = -1;
    m_fFirstTime = true; // for writing silence to the output for the first time

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
	g_Log.Error( "failed to open " + sDevName + "!\n" );
	exit( -1 );
    }

    int devcaps;
    if ( ioctl( fd, SNDCTL_DSP_GETCAPS, &devcaps ) == -1 )
    {
        g_Log.Error( "SNDCTL_DSP_GETCAPS: " + sDevName + "\n" );
	exit( -1 );
    }

    if( ( nMode == SNDCARDMODE_OUT ) && !( devcaps & PCM_CAP_OUTPUT ) )
    {
	g_Log.Error( m_sDevName + " doesn't support output\n" );
	exit( -1 );
    }
/*
    if( ( nMode == SNDCARDMODE_IN ) && !( devcaps & PCM_CAP_INPUT ) )
    {
	g_Log.Error( m_sDevName + " doesn't support input\n" );
	exit( -1 );
    }
*/
    int frag = 0x7fff000a; // Unlimited number of 1k fragments
    if( ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &frag ) == -1 )
    {
	g_Log.Error( "SNDCTL_DSP_SETFRAGMENT: " + sDevName + "\n" );
	exit ( -1 );
    }

    if( m_nMode == SNDCARDMODE_DUPLEX )
    {
	if( !( devcaps & PCM_CAP_DUPLEX ) )
	{
	    g_Log.Error( sDevName + " doesn't support one device based full duplex scheme. Please use the two device scheme.\n" );
	    exit( -1 );
	}

	if( ioctl( fd, SNDCTL_DSP_SETDUPLEX, NULL ) == -1 )
	{
    	    g_Log.Error( "can't set full duplex mode on " + sDevName + "\n" );
	    exit( -1 );
	}
    }

    int tmp = m_nChannels;
    if( ioctl( fd, SNDCTL_DSP_CHANNELS, &tmp ) == -1 )
    {
	g_Log.Error( "SNDCTL_DSP_CHANNELS: " + sDevName + "\n" );
	exit( -1 );
    }

    if( tmp != m_nChannels )
    {
	char etmp[100];
	sprintf( etmp, "can't open %d channels on %s\n", m_nChannels, sDevName.c_str() );
	g_Log.Error( etmp );
	exit( -1 );
    }

    tmp = m_nFormat;
    if( ioctl( fd, SNDCTL_DSP_SETFMT, &tmp ) == -1 )
    {
	g_Log.Error( "can't set audio format for " + sDevName + "\n" );
	exit( -1 );
    }

    if ( tmp != m_nFormat )
    {
	g_Log.Error( sDevName + " doesn't support the given sample format\n" );
        exit( -1 );
    }

    tmp = m_nRate;
    if ( ioctl( fd, SNDCTL_DSP_SPEED, &tmp ) == -1 )
    {
	char etmp[100];
	sprintf( etmp, "can't set requested (%d) sample rate on %s\n", m_nRate, sDevName.c_str() );
        g_Log.Error( etmp );
	exit( -1 );
    }

    if ( tmp != m_nRate )
    {
	char etmp[100];
	sprintf( etmp, "%s does not support requested sample rate (%d)\n", sDevName.c_str(), m_nRate );
        g_Log.Error( etmp );
	exit( -1 );
    }

    if ( ioctl( fd, SNDCTL_DSP_GETBLKSIZE, &m_nFragSize ) == -1 )
    {
        g_Log.Error( "SNDCTL_DSP_GETBLKSIZE: " + sDevName + "\n" );
	exit( -1 );
    }

    m_pBuffer = new char[ m_nBufferSize ];
    memset( m_pBuffer, 0, m_nBufferSize );

    if ( (int)m_nFragSize > m_nBufferSize )
    {
	char etmp[100];
	sprintf( etmp, "too large fragment size (%d) for current buffer size (%d) for %s\n", m_nFragSize, m_nBufferSize, sDevName.c_str() );
        g_Log.Error( etmp );
	exit( -1 );
    }

    switch( m_nMode )
    {
	case SNDCARDMODE_OUT:
	    m_nFDOut = fd;
	    break;
	case SNDCARDMODE_IN:
	    m_nFDIn = fd;
	    break;
	default:
	    m_nFDOut = m_nFDIn = fd;
	    break;
    }

    Read( tmp ); // starting sound card, if we don't do this, select() will timeout
    Stop();

    char etmp[100];
    sprintf( etmp, "Initialized %s using fragment size %d.\n", m_sDevName.c_str(), m_nFragSize );
    g_Log.Msg( etmp );
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
		g_Log.Debug( "SNDCTL_DSP_SETTRIGGER OUT 1" + m_sDevName + "\n" );
	    }
	    break;
	case SNDCARDMODE_IN:
	    trig = PCM_ENABLE_INPUT;
	    if( ioctl( m_nFDIn, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.Debug( "SNDCTL_DSP_SETTRIGGER IN 1" + m_sDevName + "\n" );
	    }
	    break;
	default:
	    trig = PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.Debug( "SNDCTL_DSP_SETTRIGGER INOUT 1" + m_sDevName + "\n" );
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
		g_Log.Debug( "SNDCTL_DSP_SETTRIGGER OUT 0" + m_sDevName + "\n" );
	    }
	    break;
	case SNDCARDMODE_IN:
	    if( ioctl( m_nFDIn, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.Debug( "SNDCTL_DSP_SETTRIGGER IN 0" + m_sDevName + "\n" );
	    }
	    break;
	default:
	    if( ioctl( m_nFDOut, SNDCTL_DSP_SETTRIGGER, &trig) == -1 )
	    {
		g_Log.Debug( "SNDCTL_DSP_SETTRIGGER INOUT 0" + m_sDevName + "\n" );
	    }
    }
}

char* CSNDCard::Read( int& nLength )
{
    struct audio_buf_info info;
    if( ioctl( m_nFDIn, SNDCTL_DSP_GETISPACE, &info ) == -1 )
    {
	g_Log.Error( "GETISPACE " + m_sDevName + "\n" );
	exit( -1 );
    }

    int n = info.bytes; // how much to read

    if( read( m_nFDIn, m_pBuffer, n ) != n )
    {
	g_Log.Error( "error reading audio data from " + m_sDevName + "\n" );
	exit( -1 );
    }

    nLength = n;

    return m_pBuffer;
}

void CSNDCard::Write( char* pBuffer, int nLength )
{
    if( m_fFirstTime)
    {
	char* pSilence = new char[ nLength ];
	memset( pSilence, 0, nLength );
	if( write( m_nFDOut, pSilence, nLength ) != nLength )
	{
	    g_Log.Error( "error writing audio data to " + m_sDevName + "\n" );
	    exit( -1 );
        }
	SAFE_DELETE_ARRAY( pSilence );
	m_fFirstTime = false;
    }

    if( write( m_nFDOut, pBuffer, nLength ) != (int)nLength )
    {
	g_Log.Error( "error writing audio data to " + m_sDevName + "\n" );
	exit( -1 );
    }
}

int CSNDCard::GetFDIn()
{
    return m_nFDIn;
}
