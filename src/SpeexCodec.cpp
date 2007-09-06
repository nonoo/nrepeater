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

#include "SpeexCodec.h"
#include "Main.h"

#include <speex/speex_header.h>

void CSpeexCodec::generateHeader()
{
    SpeexHeader SpxHdr;
    speex_init_header( &SpxHdr, m_nSampleRate, m_nChannels, &speex_nb_mode );
    SpxHdr.vbr = 1;
    SpxHdr.nb_channels = m_nChannels;
    SpxHdr.frames_per_packet = 1;
    int nPacketSize;
    m_Op.packet = (unsigned char *)speex_header_to_packet( &SpxHdr, &nPacketSize );
    m_Op.b_o_s = 1;
    m_Op.e_o_s = 0;
    m_Op.bytes = nPacketSize;
    m_Op.granulepos = 0;

    m_pOgg->feedPacket( &m_Op, true );

    free( m_Op.packet );
}

void CSpeexCodec::generateComment()
{
    char* vendor_string = PACKAGE_STRING;
    unsigned int vendor_length = strlen( PACKAGE_STRING );
    unsigned int user_comment_list_length = 0;
    unsigned int nPacketSize = vendor_length+sizeof(unsigned int)+sizeof(unsigned int);

    m_Op.packet = (unsigned char *)malloc( nPacketSize );
    memcpy( m_Op.packet, &vendor_length, sizeof(unsigned int) );
    memcpy( m_Op.packet+sizeof(unsigned int), vendor_string, vendor_length );
    memcpy( m_Op.packet+sizeof(unsigned int)+vendor_length, &user_comment_list_length, sizeof(unsigned int) );

    m_Op.b_o_s = 0;
    m_Op.e_o_s = 0;
    m_Op.granulepos = 0;

    m_Op.bytes = nPacketSize;

    m_pOgg->feedPacket( &m_Op, true );

    free( m_Op.packet );
}

CSpeexCodec::CSpeexCodec()
{
    m_pState = NULL;
    m_pPreProcState = NULL;
    m_pOut = NULL;
    m_pOgg = NULL;
    m_nSampleRate = 0;
    m_nChannels = 0;
}

void CSpeexCodec::initEncode( COggOutStream* pOgg, int nSampleRate, int nChannels, int nBitRate )
{
    m_bDecoder = false;
    m_pOgg = pOgg;
    m_nSampleRate = nSampleRate;
    m_nChannels = nChannels;
    m_pState = speex_encoder_init( &speex_nb_mode );

    // setting encoder options
    int tmp = nBitRate;
    //speex_encoder_ctl( m_pState, SPEEX_SET_ABR, &tmp);
    speex_encoder_ctl( m_pState, SPEEX_SET_BITRATE, &tmp);
    tmp = 1;
    speex_encoder_ctl( m_pState, SPEEX_SET_VBR, &tmp);
    speex_encoder_ctl( m_pState, SPEEX_SET_DTX, &tmp);
    //tmp = 10;
    //speex_encoder_ctl( m_pState, SPEEX_SET_VBR_QUALITY, &tmp);
    tmp = 10;
    speex_encoder_ctl( m_pState, SPEEX_SET_COMPLEXITY, &tmp);

    speex_encoder_ctl( m_pState, SPEEX_GET_FRAME_SIZE, &m_nFrameSize);

    speex_encoder_ctl( m_pState, SPEEX_SET_SAMPLING_RATE, &m_nSampleRate );
    speex_encoder_ctl( m_pState, SPEEX_GET_SAMPLING_RATE, &tmp );

    m_pPreProcState = speex_preprocess_state_init( m_nFrameSize, tmp );

    // setting preprocessor options
    tmp = 1;
    speex_preprocess_ctl( m_pPreProcState, SPEEX_PREPROCESS_SET_DENOISE, &tmp );
    speex_preprocess_ctl( m_pPreProcState, SPEEX_PREPROCESS_SET_AGC, &tmp );
    speex_preprocess_ctl( m_pPreProcState, SPEEX_PREPROCESS_SET_VAD, &tmp );

    speex_bits_init( &m_spxBits );

    m_pOut = new char[ m_nFrameSize * 2 ];
    m_lGranulePos = 0;

    generateHeader();
    generateComment();
}

/*void CSpeexCodec::initDecode( int nSampleRate, int nChannels )
{
    m_bDecoder = true;
    m_pOgg = pOgg;
    m_nSampleRate = nSampleRate;
    m_nChannels = nChannels;
    m_pState = speex_decoder_init( &speex_nb_mode );

    // setting decoder options
    int tmp = 1;
    speex_decoder_ctl( m_State, SPEEX_SET_ENH, &tmp );
    speex_decoder_ctl( m_State, SPEEX_GET_FRAME_SIZE, &m_nFrameSize );
	
    speex_bits_init( &m_spxBits );

    m_pOut = new char[ m_nFrameSize * 2 ];
}*/

void CSpeexCodec::destroy()
{
    speex_bits_destroy( &m_spxBits );
    if( m_pState != NULL )
    {
	if( m_bDecoder )
	{
	    speex_decoder_destroy( m_pState );
	}
	else
	{
	    speex_encoder_destroy( m_pState ); 
	}
    }
    if( m_pPreProcState != NULL )
    {
	speex_preprocess_state_destroy( m_pPreProcState );
    }

    SAFE_DELETE_ARRAY( m_pOut );

}

CSpeexCodec::~CSpeexCodec()
{
    destroy();
}

void CSpeexCodec::encode( short* pData, int nFramesNum )
{
    int nDataPos = 0;

    do
    {
	speex_preprocess( m_pPreProcState, pData + nDataPos, NULL);
	speex_bits_reset( &m_spxBits );
	speex_encode_int( m_pState, pData + nDataPos, &m_spxBits );
	nDataPos += m_nFrameSize;
	unsigned int nBytesOut = speex_bits_write( &m_spxBits, m_pOut, m_nFrameSize * 2 );

	m_Op.packet = (unsigned char*)m_pOut;
	m_Op.bytes = nBytesOut;
	m_Op.b_o_s = 0;
	m_Op.e_o_s = 0;
	m_Op.granulepos = ++m_lGranulePos;
	m_Op.granulepos = -1;

	m_pOgg->feedPacket( &m_Op, false );

    } while( nDataPos < nFramesNum );
}

/*void CSpeexCodec::decode( short* pData, int nFramesNum )
{
    speex_bits_read_from( &m_spxBits, m_pData, nFramesNum * 2 );
    speex_decode( m_pState, &m_spxBits, m_pOut );
}*/
