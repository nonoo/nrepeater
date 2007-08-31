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

#ifndef __CPARPORT_H
#define __CPARPORT_H

#include <parapin.h>

class CParPort
{
public:
    CParPort( int port );
    void init();
    void setReceiverPin( int nPin );
    void setReceiverLow( bool fLow );
    void setTransmitterPin1( int nPin );
    void setTransmitterPin2( int nPin );
    void setPTT( bool fState );
    bool isSquelchOff();

private:
    int m_nReceiverPin;
    bool m_fReceiverLow;
    int m_nTransmitterPin1;
    int m_nTransmitterPin2;
};

#endif
