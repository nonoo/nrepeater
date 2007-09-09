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

#ifndef __SNDCARD_H
#define __SNDCARD_H

#define SNDCARDMODE_IN 0
#define SNDCARDMODE_OUT 1
#define SNDCARDMODE_DUPLEX 2

#include <string>

class CSNDCard {
public:
    CSNDCard( std::string szDevName, int nMode, int nRate, int nChannels );
    ~CSNDCard();

    void	stop();
    void	start();

    short*	read( int& nLength);
    void	write( short* pBuffer, int nLength );

    int		getFDIn();
    int		getBufferSize();
    int		getSampleRate();
    int		getChannelNum();

private:
    std::string		m_szDevName;
    int			m_nFDOut, m_nFDIn;
    int			m_nMode;
    short*		m_pBuffer;
    int			m_nBufferSize;
    int			m_nRate;
    int			m_nChannels;
    int			m_nFormat;
    unsigned int	m_nFragSize;
    bool		m_bFirstTime;
};

#endif
