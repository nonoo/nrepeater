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

#include "OggFileOutStream.h"
#include "Main.h"

#include <stdio.h>

using namespace std;

COggFileOutStream::COggFileOutStream( unsigned int nSerial ) : COggOutStream( nSerial )
{
}

COggFileOutStream::~COggFileOutStream()
{
    close();
}

void COggFileOutStream::feedPacket( ogg_packet* m_Op, bool bFlush )
{
    m_Op->packetno = m_lPacketNo++;

    ogg_stream_packetin( m_pStreamState, m_Op );

    if( ogg_stream_pageout( m_pStreamState, &m_OggPage ) )
    {
	writePage();
    }
    else
    {
	if( bFlush )
	{
	    ogg_stream_flush( m_pStreamState, &m_OggPage );

	    writePage();
	}
    }
}

void COggFileOutStream::open( string sFileName )
{
    m_pFile = fopen( sFileName.c_str(), "w" );
}

void COggFileOutStream::close()
{
    ogg_stream_flush( m_pStreamState, &m_OggPage );

    writePage();

    fclose( m_pFile );
}

void COggFileOutStream::writePage()
{
    fwrite( m_OggPage.header, 1, m_OggPage.header_len, m_pFile );
    fwrite( m_OggPage.body, 1, m_OggPage.body_len, m_pFile );
}
