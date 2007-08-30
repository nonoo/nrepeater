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

CLog g_Log;
CLoop g_Loop;
CSettingsFile g_MainConfig;
CParPort* g_ParPort = NULL;
CSNDCard* g_SNDCardIn = NULL;
CSNDCard* g_SNDCardOut = NULL;

int main( int argc, char* argv[] )
{
    g_Log.Msg2( PACKAGE + string( " v" ) + PACKAGE_VERSION + string( " by Nonoo <nonoo@nonoo.hu>\n" ) );
    g_Log.Msg2( "http://www.nonoo.hu/projects/nrepeater/\n\n" );

    g_MainConfig.SetConfigFile( string( PACKAGE ) + ".conf" );
    g_MainConfig.LoadConfig();
    g_ParPort = new CParPort( LPT1 );

    // initializing sound card(s)
    string sSNDDevIn = g_MainConfig.Get( "sound", "dev_in", "/dev/dsp" );
    string sSNDDevOut = g_MainConfig.Get( "sound", "dev_out", "/dev/dsp2" );

    if( sSNDDevIn == sSNDDevOut )
    {
	g_SNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_DUPLEX );
	g_SNDCardOut = g_SNDCardIn;
    }
    else
    {
	g_SNDCardIn = new CSNDCard( sSNDDevIn, SNDCARDMODE_IN );
	g_SNDCardOut = new CSNDCard( sSNDDevOut, SNDCARDMODE_OUT );
    }

    // starting main loop
    g_Loop.Start();


    SAFE_DELETE( g_ParPort );
    SAFE_DELETE( g_SNDCardIn );
    SAFE_DELETE( g_SNDCardOut );
    return 0;
}
