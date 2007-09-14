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
#include "SpeechSynth.h"
#include "SNDCard.h"

extern "C" cst_voice *register_cmu_us_kal();
extern "C" void unregister_cmu_us_kal( cst_voice_struct* );
extern CSNDCard* g_pSNDCardOut;

CSpeechSynth::CSpeechSynth()
{
    flite_init();

    m_pVoice = register_cmu_us_kal();
    m_pWaveOut = NULL;

    m_Resampler.init( (float)g_pSNDCardOut->getSampleRate() / 8000, 1 );
}

short* CSpeechSynth::synthetize( string szText, int& nFramesOut )
{
    if( m_pWaveOut != NULL )
    {
	free( m_pWaveOut->samples );
	free( m_pWaveOut );
    }
    m_pWaveOut = flite_text_to_wave( szText.c_str(), m_pVoice );

    // increasing volume
    for( int i = 0; i < m_pWaveOut->num_samples; i++ )
    {
	m_pWaveOut->samples[i] = (short)( m_pWaveOut->samples[i] * 1.8 );
    }

    // resampling
    nFramesOut = 0;
    m_pResampledBuf = m_Resampler.resample( m_pWaveOut->samples, m_pWaveOut->num_samples, nFramesOut );
    return m_pResampledBuf;
}

CSpeechSynth::~CSpeechSynth()
{
    unregister_cmu_us_kal( m_pVoice );

    if( m_pWaveOut != NULL )
    {
	free( m_pWaveOut->samples );
	free( m_pWaveOut );
    }
}
