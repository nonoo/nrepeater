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

#ifndef __SETTINGSFILE_H
#define __SETTINGSFILE_H

#include <string>
#include <map>

// config file settings manager class
//
class CSettingsFile
{
public:
    CSettingsFile();

    // if config path is not set, loadconfig tries to
    // figure out where the config file is
    //
    void	SetConfigPath( std::string szConfigPath );
    void	SetConfigFile( std::string szConfigFile );

    void	LoadConfig();
    void	SaveConfig();

    std::string	Get( std::string Section, std::string Key, std::string DefaultValue );
    int		GetInt( std::string Section, std::string Key, const int& DefaultValue );
    void	Set( std::string Section, std::string Key, std::string Value );

private:
    // removes whitespaces
    //
    std::string	TrimLeft( std::string szString );
    std::string	TrimRight( std::string szString );

    void	SearchForConfigFile();




    std::string	m_szInitialHomeDir;
    std::string	m_szConfigFile;
    std::string	m_szConfigPath;

    // ini structure in memory
    // m_Settings[section][key] == value
    //
    typedef std::map< std::string, std::string >	t_mKeys;
    std::map< std::string, t_mKeys >			m_Settings;
};

#endif
