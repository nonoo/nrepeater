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

#ifndef __OGGOUTSTREAM_H
#define __OGGOUTSTREAM_H

#include <ogg/ogg.h>
#include <string>

class COggOutStream
{
public:
    COggOutStream( unsigned int nSerial );
    virtual ~COggOutStream();

    virtual void feedPacket( ogg_packet* m_Op, bool bFlush );
    virtual void open( std::string szFileName );
    virtual void close();
    unsigned int getSerial();

protected:
    unsigned int	m_nSerial;
	
    ogg_stream_state*	m_pStreamState;
    ogg_page		m_OggPage;

    long		m_lPacketNo;
};

#endif
