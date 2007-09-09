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
#include "Log.h"
#include "ParPort.h"
#include "Loop.h"
#include "SettingsFile.h"
#include "SNDCard.h"
#include "Archiver.h"

#include <csignal>
#include <getopt.h>

// argument list
const struct option long_options[] =
{
    { "help",	0,	NULL,	'h' },
    { "config",	1,	NULL,	'c' },
    { NULL,	0,	NULL,	0 }
};
const char* const short_options = "hc:";

CLog		g_Log;
CLoop		g_Loop;
CSettingsFile	g_MainConfig;
CParPort*	g_ParPort = NULL;
CSNDCard*	g_SNDCardIn = NULL;
CSNDCard*	g_SNDCardOut = NULL;
bool		g_fTerminate = false;
CArchiver	g_Archiver;
bool		g_fDaemonMode = false;
string		g_szConfigFile = string( PACKAGE_NAME ) + ".conf";

void daemonize()
{
    int res = fork();
    if( res < 0 )
    {
        g_fDaemonMode = false;
        g_Log.log( CLOG_ERROR, "can't fork into the background!\n" );
        return;
    }
    if( res > 0 )
    {
        // parent exiting
        exit( EXIT_SUCCESS );
    }

    // become a process group leader
    setsid();

    // ignoring SIGCHLD
    signal( SIGCHLD, SIG_IGN );

    chdir( "/" );

    g_Log.log( CLOG_MSG, "forked into the background\n" );
}

void initParPort()
{
    string sParPort = g_MainConfig.get( "lpt", "port", "LPT1" );
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

    g_ParPort->setReceiverPin( g_MainConfig.getInt( "lpt", "receiver_pin", 11 ) );
    g_ParPort->setReceiverLow( g_MainConfig.getInt( "lpt", "receiver_low", 1 ) );
    g_ParPort->setTransmitterPin1( g_MainConfig.getInt( "lpt", "transmitter_pin1", 2 ) );
    g_ParPort->setTransmitterPin2( g_MainConfig.getInt( "lpt", "transmitter_pin2", 3 ) );

    g_ParPort->init();
}

void initSndCards()
{
    string sSNDDevIn = g_MainConfig.get( "sound", "dev_in", "/dev/dsp" );
    string sSNDDevOut = g_MainConfig.get( "sound", "dev_out", "/dev/dsp2" );

    if( sSNDDevIn == sSNDDevOut )
    {
	g_SNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_DUPLEX, g_MainConfig.getInt( "sound", "rate", 44100 ), g_MainConfig.getInt( "sound", "channels", 1 ) );
	g_SNDCardOut = g_SNDCardIn;
    }
    else
    {
	g_SNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_IN, g_MainConfig.getInt( "sound", "rate", 44100 ), g_MainConfig.getInt( "sound", "channels", 1 ) );
	g_SNDCardOut = new CSNDCard( sSNDDevOut, SNDCARDMODE_OUT, g_MainConfig.getInt( "sound", "rate", 44100 ), g_MainConfig.getInt( "sound", "channels", 1 ) );
    }
}

void atExit()
{
    g_Log.log( CLOG_MSG, "exiting.\n\n" );
}

void onSIGTERM( int )
{
    g_fTerminate = true;
}

void onSIGHUP( int )
{
    // reloading config file
    g_MainConfig.loadConfig();
}

void printUsage( string szProgName )
{
    cout << string( PACKAGE ) + " v" + string( PACKAGE_VERSION ) + " by Nonoo <nonoo@nonoo.hu>" << endl;
    cout << "http://www.nonoo.hu/projects/nrepeater/" << endl << endl;
    cout << "Usage:  " << szProgName << " options" << endl << endl;
    cout << "    -h  --help" << endl;
    cout << "    -c  --config filename   Use given config file path" << endl << endl;
    exit( EXIT_SUCCESS );
}

void parseCommandLine( int argc, char* argv[] )
{
    int next_option;

    do
    {
	next_option = getopt_long( argc, argv, short_options, long_options, NULL );

	switch( next_option )
	{
	    case 'h':
		printUsage( argv[0] );
	    case 'c':
		g_szConfigFile = optarg;
		break;
	    default:
		break;
	}
    } while( next_option != -1 );
}

int main( int argc, char* argv[] )
{
    parseCommandLine( argc, argv );

    g_MainConfig.setConfigFile( g_szConfigFile );
    g_MainConfig.loadConfig();

    g_Log.setScreenLogLevel( g_MainConfig.getInt( "logging", "loglevel_screen", 1 ) );
    g_Log.setSysLogLevel( g_MainConfig.getInt( "logging", "loglevel_syslog", 1 ) );
    g_Log.setFileLogLevel( g_MainConfig.getInt( "logging", "loglevel_logfile", 0 ) );

    g_Log.log( CLOG_MSG | CLOG_NO_TIME_DISPLAY, PACKAGE + string( " v" ) + PACKAGE_VERSION + string( " by Nonoo <nonoo@nonoo.hu>\n" ) );
    g_Log.log( CLOG_MSG | CLOG_NO_TIME_DISPLAY, "http://www.nonoo.hu/projects/nrepeater/\n\n" );

    char tmp[50];
    time_t tt = time( NULL );
    strftime( tmp, 50, "%Y/%m/%d", localtime( &tt ) );
    g_Log.log( CLOG_MSG, "* starting " + string( PACKAGE_NAME ) + " on " + string( tmp ) + "\n" );

    if( g_MainConfig.getInt( "daemon", "daemon_mode", 0 ) )
    {
	daemonize();
    }

    atexit( atExit );
    signal( SIGTERM, onSIGTERM );
    signal( SIGINT, onSIGTERM );
    signal( SIGHUP, onSIGHUP );

    initParPort();

    initSndCards();

    g_Archiver.init( SPEEX_SAMPLERATE, g_SNDCardOut->getChannelNum() );

    // starting main loop
    g_Loop.start();


    SAFE_DELETE( g_ParPort );
    SAFE_DELETE( g_SNDCardIn );
    SAFE_DELETE( g_SNDCardOut );

    g_Log.log( CLOG_MSG, "exiting with errcode = 0\n" );
    return EXIT_SUCCESS;
}
