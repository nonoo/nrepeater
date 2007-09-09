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

#define CLOG_MSG		1
#define CLOG_ERROR		2
#define CLOG_WARNING		4
#define CLOG_DEBUG		8
#define CLOG_DEBUG_EXTREME	16
#define CLOG_NO_TIME_DISPLAY	32
#define CLOG_TO_ARCHIVER	64

class CLog
{
public:
    CLog();
    ~CLog();

    void log( int nFlags, std::string szMsg );
    void setScreenLogLevel( int nLogLevel );
    void setSysLogLevel( int nLogLevel );
    void setFileLogLevel( int nLogLevel );

private:
    std::string CurrTime();

    int		m_nScreenLogLevel;
    int		m_nSysLogLevel;
    int		m_nFileLogLevel;

    bool	m_bDispScreen;
    bool	m_bDispSys;
    bool	m_bDispLogFile;

    FILE*	m_pLogFile;

    int		m_nSysFlags;
};

#endif
