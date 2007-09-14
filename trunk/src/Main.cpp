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
    { "help",		0,	NULL,	'h' },
    { "config",		1,	NULL,	'c' },
    { "version",	0,	NULL,	'v' },
    { "version-num",	0,	NULL,	'V' },
    { NULL,		0,	NULL,	0 }
};
const char* const short_options = "hc:vV";

CLog		g_Log;
CLoop*		g_pLoop;
CSettingsFile	g_MainConfig;
CParPort*	g_pParPort = NULL;
CSNDCard*	g_pSNDCardIn = NULL;
CSNDCard*	g_pSNDCardOut = NULL;
bool		g_fTerminate = false;
CArchiver	g_Archiver;
bool		g_bDaemonMode = false;
string		g_szConfigFile = string( PACKAGE_NAME ) + ".conf";

void daemonize()
{
    int res = fork();
    if( res < 0 )
    {
        g_bDaemonMode = false;
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
	g_pParPort = new CParPort( LPT1 );
    }
    else
    {
	if( sParPort == "LPT2" )
	{
	    g_pParPort = new CParPort( LPT2 );
	}
	else
	{
	    g_pParPort = new CParPort( atoi( sParPort.c_str() ) );
	}
    }

    g_pParPort->setReceiverPin( g_MainConfig.getInt( "lpt", "receiver_pin", 11 ) );
    g_pParPort->setReceiverLow( g_MainConfig.getInt( "lpt", "receiver_low", 1 ) );
    g_pParPort->setTransmitterPin1( g_MainConfig.getInt( "lpt", "transmitter_pin1", 2 ) );
    g_pParPort->setTransmitterPin2( g_MainConfig.getInt( "lpt", "transmitter_pin2", 3 ) );

    g_pParPort->init();
}

void initSndCards()
{
    string sSNDDevIn = g_MainConfig.get( "sound", "dev_in", "/dev/dsp" );
    string sSNDDevOut = g_MainConfig.get( "sound", "dev_out", "/dev/dsp2" );

    if( sSNDDevIn == sSNDDevOut )
    {
	g_pSNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_DUPLEX, g_MainConfig.getInt( "sound", "rate", 44100 ), g_MainConfig.getInt( "sound", "channels", 1 ) );
	g_pSNDCardOut = g_pSNDCardIn;
    }
    else
    {
	g_pSNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_IN, g_MainConfig.getInt( "sound", "rate", 44100 ), g_MainConfig.getInt( "sound", "channels", 1 ) );
	g_pSNDCardOut = new CSNDCard( sSNDDevOut, SNDCARDMODE_OUT, g_MainConfig.getInt( "sound", "rate", 44100 ), g_MainConfig.getInt( "sound", "channels", 1 ) );
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

void printHeader()
{
    cout << string( PACKAGE ) + " v" + string( PACKAGE_VERSION ) + " by Nonoo <nonoo@nonoo.hu>" << endl;
    cout << "http://www.nonoo.hu/projects/nrepeater/" << endl << endl;
}

void printUsage( string szProgName )
{
    printHeader();
    cout << "Usage:  " << szProgName << " options" << endl << endl;
    cout << "    -h  --help" << endl;
    cout << "    -c  --config filename   Use given config file path" << endl << endl;
    exit( EXIT_SUCCESS );
}

static void parseCommandLine( int argc, char* argv[] )
{
    int next_option;

    do
    {
	next_option = getopt_long( argc, argv, short_options, long_options, NULL );

	switch( next_option )
	{
	    case 'v':
		printHeader();
		exit( EXIT_SUCCESS );
	    case 'V':
		cout << PACKAGE_VERSION << endl;
		exit( EXIT_SUCCESS );
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

void mainInit()
{
    SAFE_DELETE( g_pParPort );
    SAFE_DELETE( g_pSNDCardIn );
    SAFE_DELETE( g_pSNDCardOut );

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

    if( g_MainConfig.getInt( "daemon", "daemon_mode", 0 ) && !g_bDaemonMode )
    {
	daemonize();
    }

    atexit( atExit );
    signal( SIGTERM, onSIGTERM );
    signal( SIGINT, onSIGTERM );
    signal( SIGHUP, SIG_IGN );

    initParPort();

    initSndCards();

    g_Archiver.init( SPEEX_SAMPLERATE, g_pSNDCardOut->getChannelNum() );

    g_pLoop = new CLoop();
}

int main( int argc, char* argv[] )
{
    parseCommandLine( argc, argv );

    g_MainConfig.setConfigFile( g_szConfigFile );

    mainInit();

    // starting main loop
    g_pLoop->start();

    SAFE_DELETE( g_pLoop );
    SAFE_DELETE( g_pParPort );
    SAFE_DELETE( g_pSNDCardIn );
    SAFE_DELETE( g_pSNDCardOut );

    g_Log.log( CLOG_MSG, "exiting with errcode = 0\n" );
    return EXIT_SUCCESS;
}
