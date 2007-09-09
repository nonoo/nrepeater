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
#include "DTMF.h"
#include "SettingsFile.h"

extern CSettingsFile g_MainConfig;

void CDTMF::init( int nSampleRate )
{
    m_nSampleRate = nSampleRate;

    m_Goertzel.init( m_nSampleRate );
}

void CDTMF::process( short* pData, int nFramesNum )
{
    if( !g_MainConfig.getInt( "dtmf", "enabled", 0 ) )
    {
	return;
    }

    m_caDecoded = m_Goertzel.process( pData, nFramesNum );

    if( m_caDecoded != NULL )
    {
	cout << "DTMF: " << m_caDecoded << endl;
    }
}
