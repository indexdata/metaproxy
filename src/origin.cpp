/* This file is part of Metaproxy.
   Copyright (C) Index Data

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
#include <assert.h>
#include <metaproxy/origin.hpp>
#include <yaz/log.h>
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
    // assume first call is immediate reverse IP: + bind IP
    // 2nd call might be X-Forwarded .. we use that for logging
    std::string tmp = m_address;
    m_address = addr;
    if (tmp.length())
    {
        m_address.append(" ");
        m_address.append(tmp);
    }
    else
    {
        size_t pos = addr.find(' ');
        assert (pos != std::string::npos);
    }
    m_origin_id = s;
}

void mp::Origin::set_custom_session(const std::string &s)
{
    m_custom_session = s;
}

std::string mp::Origin::get_forward_address() const
{
    // return first component of address
    // That's either first part of X-Forwarded component
    size_t pos = m_address.find(' ');
    if (pos != std::string::npos)
        return m_address.substr(0, pos);
    else
        return m_address;
}

std::string mp::Origin::get_address()
{
    // return 2nd last component of address (last is listening IP)
    size_t pos2 = m_address.rfind(' ');
    if (pos2 != std::string::npos && pos2 > 0)
    {
        size_t pos1 = m_address.rfind(' ', pos2 - 1);
        if (pos1 != std::string::npos)
            return m_address.substr(pos1 + 1, pos2 - pos1 - 1);
        else
            return m_address.substr(0, pos2);
    }
    else
        return m_address;
}

std::string mp::Origin::get_bind_address()
{
    // return last component of address
    size_t pos2 = m_address.rfind(' ');
    if (pos2 != std::string::npos && pos2 > 0)
    {
        return m_address.substr(pos2 + 1);
    }
    else
        return m_address;
}


std::ostream& std::operator<<(std::ostream& os, const mp::Origin& o)
{
    // print first component of address
    std::string a = o.get_forward_address();
    if (a.length())
        os << a;
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

