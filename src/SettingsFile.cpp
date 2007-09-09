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
#include "SettingsFile.h"
#include "Log.h"

#include <fstream>

using namespace std;

extern CLog g_Log;

void CSettingsFile::set( string szSection, string szKey, string szValue )
{
    m_Settings[ szSection ][ szKey ] = szValue;
}

string CSettingsFile::get( string szSection, string szKey, string szDefaultValue )
{
    if( m_Settings.count( szSection ) > 0 )
    {
	if( m_Settings[ szSection ].count( szKey ) > 0 )
	{
	    return m_Settings[ szSection ][ szKey ];
	}
    }
    g_Log.log( CLOG_DEBUG, "Warning: invalid value in config file section '" + szSection + "', key '" + szKey + "': " + m_Settings[ szSection ][ szKey ] + " - using default value: " + szDefaultValue + "\n" );
    return szDefaultValue;
}

int CSettingsFile::getInt( string szSection, string szKey, const int& nDefaultValue )
{
    if( m_Settings.count( szSection ) > 0 )
    {
	if( m_Settings[ szSection ].count( szKey ) > 0 )
	{
	    if( strcasecmp( m_Settings[ szSection ][ szKey ].c_str(), "yes" ) == 0 )
	    {
		return 1;
	    }
	    if( strcasecmp( m_Settings[ szSection ][ szKey ].c_str(), "no" ) == 0 )
	    {
		return 0;
	    }

	    char* p;
	    int res = strtol( m_Settings[ szSection ][ szKey ].c_str(), &p, 0 );
	    if( *p != 0 ) // the whole string was not valid
	    {
		char tmp[500];
		sprintf( tmp, "Warning: invalid value in config file section '%s', key '%s': %s - using default value: %d\n", szSection.c_str(), szKey.c_str(), m_Settings[ szSection ][ szKey ].c_str(), nDefaultValue );
		g_Log.log( CLOG_DEBUG, tmp );
		return nDefaultValue;
	    }
	    return res;
	}
    }
    return nDefaultValue;
}

string CSettingsFile::trimLeft( string szString )
{
    string out="";

    if( szString.size() == 0 )
    {
	return out;
    }

    bool bStarted = false;

    for( string::iterator it = szString.begin(); it != szString.end(); it++ )
    {
	if( ( !bStarted ) && ( *it != ' ' ) )
	{
	    bStarted = true;
	}

	if( bStarted )
	{
	    out += *it;
	}
    }

    return out;
}

string CSettingsFile::trimRight( string szString )
{
    string out="";

    if( szString.size() == 0 )
    {
	return out;
    }

    // searching for the first non-space char from the end of the string
    string::iterator it = szString.end()-1;
    unsigned int i = szString.size();
    while( ( *it == ' ' ) && ( it != szString.begin() ) )
    {
	it--;
	i--;
    }

    out = szString.substr( 0, i );

    return out;
}

void CSettingsFile::loadConfig()
{
    string tmp;

    if( m_szConfigFile.size() == 0 )
    {
	g_Log.log( CLOG_ERROR, "LoadConfig(): no config file specified!\n" );
	return;
    }

    string szConfigPath = m_szConfigPath + "/" + m_szConfigFile;
    ifstream FileStream( szConfigPath.c_str() );
    if( FileStream.fail() )
    {
	FileStream.close();

	// couldn't find config file, searching for it
	if( !searchForConfigFile() )
	{
	    // config file cannot be found
	    return;
	}
	// found config file, loading it
	szConfigPath = m_szConfigPath + "/" + m_szConfigFile;
	FileStream.open( szConfigPath.c_str() );
    }


    string szCurrentSection = "";
    m_Settings.clear();


    char buf[500];
    memset( buf, 0, 500 );

    while( !FileStream.eof() )
    {
	FileStream.getline( buf, 499 );
	tmp = buf;

	tmp = trimLeft( tmp );

	if( tmp[0] == '#' ) // comments
	{
	    continue;
	}
	if( tmp.size() == 0 ) // empty lines
	{
	    continue;
	}

	if( tmp[0] == '[' ) // section
	{
	    string::size_type loc = tmp.find( ']', 0 );
	    if( loc == string::npos )
	    {
		tmp = tmp.substr( 1, tmp.size()-1 );
	    }
	    else
	    {
		tmp = tmp.substr( 1, loc-1 );
	    }

	    tmp = trimRight( trimLeft( tmp ) );

	    if( tmp.size() == 0 ) // invalid section
	    {
		continue;
	    }

	    szCurrentSection = tmp;
	    continue;
	}

	if( szCurrentSection.size() == 0 ) // we're not in a section
	{
	    continue;
	}

	// we're in a section, reading key & value pairs
	string::size_type loc = tmp.find( '=', 0 );
	if( loc == string::npos ) // no '=' in current line -> invalid line
	{
	    continue;
	}

	string key = tmp.substr( 0, loc );
	key = trimRight( key );
	if( key.size() == 0 )
	{
	    continue;
	}
	string value = tmp.substr( loc+1, tmp.size()-loc-1 );
	value = trimRight( trimLeft( value ) );
	if( value.size() == 0 )
	{
	    continue;
	}

	m_Settings[szCurrentSection][key] = value;
    }

    FileStream.close();
}

int CSettingsFile::searchForConfigFile()
{
    // opening config file in current directory
    string tmp = m_szInitialHomeDir + "/" + m_szConfigFile;
    ifstream FileStream( tmp.c_str() );
    if( FileStream.fail() )
    {
	// trying in $HOME/.PACKAGE
	FileStream.close();

	char* pHomeDir = getenv( "HOME" );
	string szHomeDir = pHomeDir;
	tmp = szHomeDir + "/." + PACKAGE + "/" + m_szConfigFile;
	free( pHomeDir );

	FileStream.open( tmp.c_str(), ios::in );
	if( FileStream.fail() )
	{
	    // trying in /etc/PACKAGE
	    FileStream.close();

	    tmp = "/etc/";
	    tmp += PACKAGE;
	    tmp += "/" + m_szConfigFile;
	    FileStream.open( tmp.c_str(), ios::in );
	    if( FileStream.fail() )
	    {
		g_Log.log( CLOG_ERROR, "can't find/open config file: " + m_szConfigFile + "\n" );

		return 0;
	    }

	    m_szConfigPath = "/etc/";
	    m_szConfigPath += PACKAGE;
	    FileStream.close();
	    return 1;
	}

	m_szConfigPath = szHomeDir + "/." + PACKAGE;
	FileStream.close();
	return 1;
    }

    m_szConfigPath = m_szInitialHomeDir;
    FileStream.close();
    return 1;
}

void CSettingsFile::setConfigFile( string szConfigFile )
{
    m_szConfigFile = szConfigFile;
}

void CSettingsFile::setConfigPath( string szConfigPath )
{
    m_szConfigPath = szConfigPath;
}

CSettingsFile::CSettingsFile()
{
    char pBuffer[500];

    getcwd( pBuffer, 500 );
    m_szInitialHomeDir = pBuffer;
}

void CSettingsFile::saveConfig()
{
    ofstream FileStream;
    string tmp;

    if( m_szConfigPath.size() == 0 )
    // we try to search directory where we can write the config
    {
	tmp = "/etc/";
	tmp += PACKAGE;
	tmp += "/" + m_szConfigFile;
	FileStream.open( tmp.c_str() );
	if( FileStream.fail() )
	{
	    FileStream.close();
	    char* pHomeDir = getenv( "HOME" );
	    string szHomeDir = pHomeDir;
	    tmp = szHomeDir + "/." + PACKAGE + "/" + m_szConfigFile;
	    free( pHomeDir );
	    FileStream.open( szHomeDir.c_str() );
	    if( FileStream.fail() )
	    {
		FileStream.close();
		tmp = m_szInitialHomeDir + "/" + m_szConfigFile;
		FileStream.open( tmp.c_str() );
		if( FileStream.fail() )
		{
		    g_Log.log( CLOG_ERROR, "can't save config file: " + m_szConfigFile + "\n" );
		    return;
		}
		m_szConfigPath = m_szInitialHomeDir;
	    }
	    else
	    {
		m_szConfigPath = szHomeDir + "/." + PACKAGE;
	    }
	}
	else
	{
	    m_szConfigPath = "/etc/";
	    m_szConfigPath += PACKAGE;
	}
    }
    else
    {
	tmp = m_szConfigPath + "/" + m_szConfigFile;
	FileStream.open( tmp.c_str() );
	if( FileStream.fail() )
	{
	    g_Log.log( CLOG_ERROR, "can't save config file: " + m_szConfigPath + "/" + m_szConfigFile + "\n" );
	    return;
	}
    }

    bool fLeadingLine = false;

    for( map<string, t_mKeys>::iterator it1 = m_Settings.begin(); it1 != m_Settings.end(); it1++ )
    {
	if( fLeadingLine )
	{
	    FileStream << endl;
	}
	fLeadingLine = true;

	FileStream << '['  << it1->first << ']' << endl;

	t_mKeys mKeys = it1->second;
	for( t_mKeys::iterator it2 = mKeys.begin(); it2 != mKeys.end(); it2++ )
	{
	    FileStream << it2->first << "=" << it2->second << endl;
	}
    }

    FileStream.close();
}
