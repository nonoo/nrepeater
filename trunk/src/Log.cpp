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

#include <iostream>
#include <string>
#include <time.h>

#include "Log.h"

void CLog::Error( string msg )
{
    cout << CurrTime() << "Error: " << msg;
}

void CLog::Warning( string msg )
{
    cout << CurrTime() << "Warning: " << msg;
}

void CLog::Msg( string msg )
{
    cout << CurrTime() << msg;
}

// msg without time
void CLog::Msg2( string msg )
{
    cout << msg;
}

void CLog::Debug( string msg )
{
    cout << CurrTime() << msg;
}

// returns the time in [H:m:s] format
string CLog::CurrTime()
{
    char tmp[50];
    time_t t = time( NULL );
    struct tm *tmpt = localtime( &t );
    strftime( tmp, 50, "%T", tmpt );
    return "[" + string( tmp ) + "] ";
}
