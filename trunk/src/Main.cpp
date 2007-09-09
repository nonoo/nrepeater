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

#include <csignal>

#include "Main.h"
#include "Log.h"
#include "ParPort.h"
#include "Loop.h"
#include "SettingsFile.h"
#include "SNDCard.h"
#include "WavFile.h"
#include "Archiver.h"

CLog		g_Log;
CLoop		g_Loop;
CSettingsFile	g_MainConfig;
CParPort*	g_ParPort = NULL;
CSNDCard*	g_SNDCardIn = NULL;
CSNDCard*	g_SNDCardOut = NULL;
CWavFile	g_RogerBeep;
bool		g_fTerminate = false;
CArchiver	g_Archiver;

void initParPort()
{
    string sParPort = g_MainConfig.Get( "lpt", "port", "LPT1" );
    if( sParPort == "LPT1" )
    {
	g_ParPort = new CParPort( LPT1 );
    }
    else
    {
	if( sParPort == "LPT2" )
	{
	    g_ParPort = new CParPort( LPT2 );
	}
	else
	{
	    g_ParPort = new CParPort( atoi( sParPort.c_str() ) );
	}
    }

    g_ParPort->setReceiverPin( g_MainConfig.GetInt( "lpt", "receiver_pin", 11 ) );
    g_ParPort->setReceiverLow( g_MainConfig.GetInt( "lpt", "receiver_low", 1 ) );
    g_ParPort->setTransmitterPin1( g_MainConfig.GetInt( "lpt", "transmitter_pin1", 2 ) );
    g_ParPort->setTransmitterPin2( g_MainConfig.GetInt( "lpt", "transmitter_pin2", 3 ) );

    g_ParPort->init();
}

void initSndCards()
{
    string sSNDDevIn = g_MainConfig.Get( "sound", "dev_in", "/dev/dsp" );
    string sSNDDevOut = g_MainConfig.Get( "sound", "dev_out", "/dev/dsp2" );

    if( sSNDDevIn == sSNDDevOut )
    {
	g_SNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_DUPLEX, g_MainConfig.GetInt( "sound", "rate", 44100 ), g_MainConfig.GetInt( "sound", "channels", 1 ) );
	g_SNDCardOut = g_SNDCardIn;
    }
    else
    {
	g_SNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_IN, g_MainConfig.GetInt( "sound", "rate", 44100 ), g_MainConfig.GetInt( "sound", "channels", 1 ) );
	g_SNDCardOut = new CSNDCard( sSNDDevOut, SNDCARDMODE_OUT, g_MainConfig.GetInt( "sound", "rate", 44100 ), g_MainConfig.GetInt( "sound", "channels", 1 ) );
    }
}

void atExit()
{
    g_Log.log( LOG_MSG, "exiting.\n\n" );
}

void onSIGTERM( int )
{
    g_fTerminate = true;
}

void onSIGHUP( int )
{
    // reloading config file
    g_MainConfig.LoadConfig();
}

int main( int argc, char* argv[] )
{
    g_MainConfig.SetConfigFile( string( PACKAGE ) + ".conf" );
    g_MainConfig.LoadConfig();

    g_Log.setScreenLogLevel( g_MainConfig.GetInt( "logging", "loglevel_screen", 1 ) );
    g_Log.setSysLogLevel( g_MainConfig.GetInt( "logging", "loglevel_syslog", 1 ) );
    g_Log.setFileLogLevel( g_MainConfig.GetInt( "logging", "loglevel_logfile", 0 ) );

    g_Log.log( LOG_MSG | LOG_NO_TIME_DISPLAY, PACKAGE + string( " v" ) + PACKAGE_VERSION + string( " by Nonoo <nonoo@nonoo.hu>\n" ) );
    g_Log.log( LOG_MSG | LOG_NO_TIME_DISPLAY, "http://www.nonoo.hu/projects/nrepeater/\n\n" );

    char tmp[50];
    time_t tt = time( NULL );
    strftime( tmp, 50, "%Y/%m/%d", localtime( &tt ) );
    g_Log.log( LOG_MSG, "* starting " + string( PACKAGE_NAME ) + " on " + string( tmp ) + "\n" );

    atexit( atExit );
    signal( SIGTERM, onSIGTERM );
    signal( SIGINT, onSIGTERM );
    signal( SIGHUP, onSIGHUP );

    initParPort();

    initSndCards();

    g_Archiver.init( SPEEX_SAMPLERATE, g_SNDCardOut->getChannelNum() );

    // loading roger beep wave file
    if( g_MainConfig.GetInt( "rogerbeep", "enabled", 1 ) )
    {
	g_RogerBeep.loadToMemory( g_MainConfig.Get( "rogerbeep", "file", "beep.wav" ) );
	if( g_RogerBeep.getSampleRate() != g_SNDCardOut->getSampleRate() )
	{
	    char errstr[100];
	    sprintf( errstr, "roger beep sample rate (%dhz) doesn't match output sample rate (%dhz)\n", g_RogerBeep.getSampleRate(), g_SNDCardOut->getSampleRate() );
	    g_Log.log( LOG_WARNING, errstr );
	}
	if( g_RogerBeep.getChannelNum() != g_SNDCardOut->getChannelNum() )
	{
	    char errstr[100];
	    sprintf( errstr, "roger beep file has %d channel(s), output has %d\n", g_RogerBeep.getChannelNum(), g_SNDCardOut->getChannelNum() );
	    g_Log.log( LOG_WARNING, errstr );
	}
	g_RogerBeep.setVolume( g_MainConfig.GetInt( "rogerbeep", "volume", 80 ) );
	g_Log.log( LOG_DEBUG, "roger beep file loaded into memory\n" );
    }

    // starting main loop
    g_Loop.Start();


    SAFE_DELETE( g_ParPort );
    SAFE_DELETE( g_SNDCardIn );
    SAFE_DELETE( g_SNDCardOut );
    return 0;
}
