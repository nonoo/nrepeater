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
#include "OggFileOutStream.h"
#include "Log.h"

#include <stdio.h>

using namespace std;

extern CLog g_Log;

COggFileOutStream::COggFileOutStream( unsigned int nSerial ) : COggOutStream( nSerial )
{
    m_pFile = NULL;
}

COggFileOutStream::~COggFileOutStream()
{
    destroy();
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
	    flush();
	}
    }
}

void COggFileOutStream::init( string szFileName )
{
    m_szFileName = szFileName;
    if( m_pStreamState == NULL )
    {
	m_pStreamState = (ogg_stream_state *) malloc( sizeof( ogg_stream_state ) );
	ogg_stream_init( m_pStreamState, m_nSerial );
    }
    m_lPacketNo = 0;

    m_pFile = fopen( m_szFileName.c_str(), "w" );
    if( !m_pFile )
    {
	g_Log.log( CLOG_ERROR, "can't open logfile for writing: " + m_szFileName + "\n" );
    }
}

void COggFileOutStream::destroy()
{
    if( m_pStreamState )
    {
	flush();

	ogg_stream_destroy( m_pStreamState );
	m_pStreamState = NULL;
    }

    if( m_pFile != NULL )
    {
	fclose( m_pFile );
    }
}

void COggFileOutStream::flush()
{
    while( ogg_stream_flush( m_pStreamState, &m_OggPage ) )
    {
        writePage();
    }
}

void COggFileOutStream::writePage()
{
    if( m_pFile != NULL )
    {
	fwrite( m_OggPage.header, 1, m_OggPage.header_len, m_pFile );
	fwrite( m_OggPage.body, 1, m_OggPage.body_len, m_pFile );
    }
}
