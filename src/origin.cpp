/* $Id: origin.cpp,v 1.6 2007-05-09 21:23:09 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

//#include "config.hpp"
#include "origin.hpp"

#include <iostream>

namespace mp = metaproxy_1;

mp::Origin::Origin(std::string listen_host, 
                   unsigned int listen_port) 
    : m_type(API), m_address(""), m_origin_id(0),
      m_listen_host(listen_host), m_listen_port(listen_port)
{
}

std::string mp::Origin::listen_host() const
{
    return m_listen_host;
};

std::string & mp::Origin::listen_host()
{
    return m_listen_host;
};

unsigned int mp::Origin::listen_port() const
{
    return m_listen_port;
};

unsigned int & mp::Origin::listen_port()
{
    return m_listen_port;
};



void mp::Origin::set_tcpip_address(std::string addr, unsigned long s)
{
    m_type = TCPIP;
    m_address = addr;
    m_origin_id = s;
}

std::ostream& std::operator<<(std::ostream& os,  mp::Origin& o)
{
    if (o.m_address != "")
        os << o.m_address;
    else
        os << "0";
    os << ":" << o.m_origin_id;
    return os;
}
                
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
