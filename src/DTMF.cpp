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
#include "DTMF.h"
#include "SettingsFile.h"
#include "Log.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/kd.h>

extern CSettingsFile	g_MainConfig;
extern CLog		g_Log;

void CDTMF::init( int nSampleRate )
{
    m_nSampleRate = nSampleRate;
    m_nSamplesProcessed = m_nPause = m_cDecoded = m_cPrevDecoded = m_nDecodedCharsNum = 0;

    memset( m_caDecodedChars, 0, sizeof( m_caDecodedChars ) );

    m_Goertzel.init( m_nSampleRate );
}

char* CDTMF::finishDecoding()
{
    if( m_nDecodedCharsNum > 0 )
    {
	return m_caDecodedChars;
    }
    else
    {
	return NULL;
    }

    m_nPause = m_nDecodedCharsNum = 0;
}

void CDTMF::clearSequence()
{
    memset( m_caDecodedChars, 0, sizeof( m_caDecodedChars ) );
    m_nDecodedCharsNum = 0;
}    

void CDTMF::process( short* pData, int nFramesNum )
{
    if( !g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	return;
    }

    m_cDecoded = m_Goertzel.process( pData, nFramesNum );

    if( m_cDecoded == 0 )
    {
	// no dtmf tone decoded
        m_nSamplesProcessed = m_cPrevDecoded = m_cDecoded = 0;

	m_nPause += nFramesNum;
	return;
    }

    // protecting m_caDecodedChars from overflow
    if( m_nDecodedCharsNum == sizeof( m_caDecodedChars ) - 1 )
    {
	return;
    }

    // beginning of decoding a new tone
    if( m_cPrevDecoded == 0 )
    {
	m_cPrevDecoded = m_cDecoded;
    }

    if( m_cDecoded != m_cPrevDecoded )
    {
        // decoding failed
        m_nSamplesProcessed = m_cPrevDecoded = m_cDecoded = 0;
	return;
    }

    m_nSamplesProcessed += nFramesNum;

    // litz alert checking
    if( g_MainConfig.getInt( "dtmf", "litz_enabled", 1 ) )
    {
	if( !strncmp( m_caDecodedChars, "P", 1 ) && ( ( (float)m_nSamplesProcessed / m_nSampleRate ) * 1000 > 3000 ) )
	{
	    g_Log.log( CLOG_DEBUG, "DTMF [0] pressed for 3 secs: P -> !, LITZ alert\n" );
	    m_caDecodedChars[0] = '!';
	}
    }

    if( ( (float)m_nSamplesProcessed / m_nSampleRate ) * 1000 > g_MainConfig.getInt( "dtmf", "long_tone_min_length", 2000 ) && !m_bLongToneDetected )
    {
	// tone pressed for more than long_tone_min_length millisecs
	// changing stored tone code to it's long equivalent
	switch( m_caDecodedChars[ m_nDecodedCharsNum - 1 ] )
	{
	    case '1':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'Q';
		break;
	    case '2':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'W';
		break;
	    case '3':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'E';
		break;
	    case '4':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'R';
		break;
	    case '5':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'T';
		break;
	    case '6':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'Z';
		break;
	    case '7':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'U';
		break;
	    case '8':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'I';
		break;
	    case '9':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'O';
		break;
	    case '0':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'P';
		break;
	    case '#':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'S';
		break;
	    case '*':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'D';
		break;
	    case 'A':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'F';
		break;
	    case 'B':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'G';
		break;
	    case 'C':
		m_caDecodedChars[ m_nDecodedCharsNum - 1 ] = 'H';
		break;
	    default:
		break;
	}
	m_bLongToneDetected = true;
	char caDbgTmp[500];
	sprintf( caDbgTmp, "tone %c held long -> %c\n", m_cDecoded, m_caDecodedChars[ m_nDecodedCharsNum -1 ] );
	g_Log.log( CLOG_DEBUG, caDbgTmp );
    }

    if( ( m_nDecodedCharsNum > 0 ) && ( ( (float)m_nPause / m_nSampleRate ) * 1000 < g_MainConfig.getInt( "dtmf", "min_pause", 500 ) ) )
    {
	// the pause between two tones has not been reached
	m_cPrevDecoded = m_cDecoded = 0;
	return;
    }

    if( ( ( (float)m_nSamplesProcessed / m_nSampleRate ) * 1300 > g_MainConfig.getInt( "dtmf", "min_tone_length", 100 ) ) ) // 1300 = little cheating to make configuration easier
    {
	if( m_nDecodedCharsNum == 0 )
	{
	    // decoding of a new dtmf sequence, clearing stored chars
	    memset( m_caDecodedChars, 0, sizeof( m_caDecodedChars ) );
	}

	m_bLongToneDetected = false;
	m_caDecodedChars[ m_nDecodedCharsNum++ ] = m_cDecoded;
	char caDbgTmp[500];
	sprintf( caDbgTmp, "DTMF tone decoded: %c\n", m_cDecoded );
	g_Log.log( CLOG_DEBUG, caDbgTmp );
	m_nSamplesProcessed = m_cPrevDecoded = m_cDecoded = m_nPause = 0;
    }
    else
    {
	m_cPrevDecoded = m_cDecoded;
    }
}

bool CDTMF::isValidSequence( char* pszSequence )
{
    return g_MainConfig.getInt( "dtmf-action-" + string( pszSequence ), "enabled", 0 );
}

bool CDTMF::processSequence( char* pszSequence )
{
    // do we have to log something?
    if( g_MainConfig.isValidKey( "dtmf-action-" + string( pszSequence ), "log" ) )
    {
	g_Log.log( CLOG_MSG, g_MainConfig.get( "dtmf-action-" + string( pszSequence ), "log", "no log message specified.\n" ) + "\n" );
    }

    // do we have to log something to the archiver?
    if( g_MainConfig.isValidKey( "dtmf-action-" + string( pszSequence ), "log_to_archiver" ) )
    {
	g_Log.log( CLOG_TO_ARCHIVER, g_MainConfig.get( "dtmf-action-" + string( pszSequence ), "log_to_archiver", "no log message specified.\n" ) + "\n" );
    }

    // do we have to exec a command?
    if( g_MainConfig.isValidKey( "dtmf-action-" + string( pszSequence ), "exec" ) )
    {
	if( system( g_MainConfig.get( "dtmf-action-" + string( pszSequence ), "exec", "" ).c_str() ) < 0 )
	{
	    return false;
	}
    }

    // do we have to play PC speaker beeps?
    if( g_MainConfig.isValidKey( "dtmf-action-" + string( pszSequence ), "pcspeaker_beep" ) )
    {
	int nFrequency = 0;
	int nDuration = 0;
	int nNum = 0;
	int nDelay = 0;
	if( sscanf( g_MainConfig.get( "dtmf-action-" + string( pszSequence ), "pcspeaker_beep", "0" ).c_str(), "%d %d %d %d", &nFrequency, &nDuration, &nNum, &nDelay ) != 4 )
	{
	    g_Log.log( CLOG_ERROR, "invalid format in config file for pcspeaker_beep in dtmf-action-" + string( pszSequence ) + "\n" );
	    return false;
	}
	else
	{
	    char szTmp[500];
	    sprintf( szTmp, "PC speaker beep: %d Hz, count: %d ms, duration: %d ms, delay: %d ms\n", nFrequency, nNum, nDuration, nDelay );
	    g_Log.log( CLOG_MSG | CLOG_TO_ARCHIVER, szTmp );
	    beep( nFrequency, nDuration, nNum, nDelay );
	}
    }
    return true;
}

// PC speaker beep
void CDTMF::beep( int nFrequency, int nDuration, int nNum, int nDelay )
{
    int nSpeakerFD = open( "/dev/tty0", O_RDWR | O_NDELAY );

    if( nSpeakerFD < 0 )
    {
    	g_Log.log( CLOG_ERROR, "can't open PC speaker device, make sure you have CONFIG_INPUT_PCSPKR enabled in your kernel.\n" );
	return;
    }

    while( nNum > 0 )
    {
	// note down
	if( ioctl( nSpeakerFD, KIOCSOUND, 1190000 / nFrequency ) == -1 )
	{
    	    g_Log.log( CLOG_ERROR, "can't beep on the PC speaker - ioctl() error.\n" );
    	    return;
	}

	// holding the note
	usleep( nDuration * 1000 );

	// turning off the speaker
	if( ioctl( nSpeakerFD, KIOCSOUND, 0 ) == -1 )
	{
    	    g_Log.log( CLOG_ERROR, "can't turn off beep on the PC speaker - ioctl() error.\n" );
    	    return;
	}

	if( --nNum > 0 )
	{
	    // delaying between beeps
	    usleep( nDelay * 1000 );
	}
    }

    close( nSpeakerFD );
}
