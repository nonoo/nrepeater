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

#include <unistd.h>
#include <fstream>

#include "Config.h"
#include "Log.h"

extern CLog g_Log;

CConfig::CConfig( string szConfigFile )
{
    m_szConfigFile = szConfigFile;

    if( SearchForConfigFile() )
    {
	m_RC.load( string( m_szConfigPath + m_szConfigFile ).c_str() );
	g_Log.Debug( string( "Loaded config file: " ) + m_szConfigPath + "/" + m_szConfigFile + "\n" );
    }
}

bool CConfig::SearchForConfigFile()
{
    // searching for the config file in current directory
    //
    char pBuffer[500];

    getcwd( pBuffer, 500 );
    string szInitialHomeDir = pBuffer;

    string tmp = szInitialHomeDir + "/" + m_szConfigFile;
    ifstream FileStream( tmp.c_str() );
    if( FileStream.fail() )
    {
	// trying in $HOME/.PACKAGE_NAME
	//
	FileStream.close();

	char* pHomeDir = getenv( "HOME" );
	string szHomeDir = pHomeDir;
	tmp = szHomeDir + "/." + PACKAGE + "/" + m_szConfigFile;
	free( pHomeDir );

	FileStream.open( tmp.c_str(), ios::in );
	if( FileStream.fail() )
	{
	    // trying in /etc/PACKAGE_NAME
	    //
	    FileStream.close();

	    tmp = "/etc/";
	    tmp += PACKAGE;
	    tmp += "/" + m_szConfigFile;
	    FileStream.open( tmp.c_str(), ios::in );
	    if( FileStream.fail() )
	    {
		g_Log.Warning( string( "can't find config file: " ) + m_szConfigFile + "\n" );

		return false;
	    }

	    m_szConfigPath = "/etc/";
	    m_szConfigPath += PACKAGE;
	    FileStream.close();
	    return true;
	}

	m_szConfigPath = szHomeDir + "/." + PACKAGE;
	FileStream.close();
	return true;
    }

    m_szConfigPath = szInitialHomeDir;
    FileStream.close();
    return true;
}
