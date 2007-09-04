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

#ifndef __CLOG_H
#define __CLOG_H

#include <string>

#define LOGLEVEL_NONE		0
#define LOGLEVEL_NORMAL		1
#define LOGLEVEL_DEBUG		2
#define LOGLEVEL_EXTREME	3

class CLog
{
public:
    CLog();

    void Error( std::string msg );
    void Warning( std::string msg );
    void Msg( std::string msg );
    void Msg2( std::string msg );
    void Debug( std::string msg );
    void Debug2( std::string msg );
    void setLogLevel( int nLogLevel );

private:
    std::string CurrTime();

    int	m_nLogLevel;
};

#endif
