/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.hpp"
#include <metaproxy/origin.hpp>

#include <iostream>

namespace mp = metaproxy_1;

mp::Origin::Origin() : m_address(""), m_origin_id(0), m_max_sockets(0)
{
}

void mp::Origin::set_max_sockets(int max_sockets)
{
    m_max_sockets = max_sockets;
}

int mp::Origin::get_max_sockets()
{
    return m_max_sockets;
}

void mp::Origin::set_tcpip_address(std::string addr, unsigned long s)
{
    m_address = addr;
    m_origin_id = s;
}

void mp::Origin::set_custom_session(const std::string &s)
{
    m_custom_session = s;
}

std::string mp::Origin::get_address()
{
    return m_address;
}

std::ostream& std::operator<<(std::ostream& os, const mp::Origin& o)
{
    if (o.m_address.length())
        os << o.m_address;
    else
        os << "0";
    os << ":" << o.m_origin_id;
    if (o.m_custom_session.length())
        os << ":" << o.m_custom_session;
    return os;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

