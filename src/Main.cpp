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
#include "WavFile.h"

CLog g_Log;
CLoop g_Loop;
CSettingsFile g_MainConfig;
CParPort* g_ParPort = NULL;
CSNDCard* g_SNDCardIn = NULL;
CSNDCard* g_SNDCardOut = NULL;
CWavFile* g_RogerBeep = NULL;

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

int main( int argc, char* argv[] )
{
    g_Log.Msg2( PACKAGE + string( " v" ) + PACKAGE_VERSION + string( " by Nonoo <nonoo@nonoo.hu>\n" ) );
    g_Log.Msg2( "http://www.nonoo.hu/projects/nrepeater/\n\n" );

    g_MainConfig.SetConfigFile( string( PACKAGE ) + ".conf" );
    g_MainConfig.LoadConfig();

    initParPort();

    initSndCards();

    if( g_MainConfig.GetInt( "rogerbeep", "enabled", 1 ) )
    {
	g_RogerBeep = new CWavFile( g_MainConfig.Get( "rogerbeep", "file", "beep.wav" ) );
    }

    // starting main loop
    g_Loop.Start();


    SAFE_DELETE( g_ParPort );
    SAFE_DELETE( g_SNDCardIn );
    SAFE_DELETE( g_SNDCardOut );
    SAFE_DELETE( g_RogerBeep );
    return 0;
}
