/* This file is part of Metaproxy.
   Copyright (C) 2005-2009 Index Data

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
#include "package.hpp"

#include <iostream>

namespace mp = metaproxy_1;

mp::Package::Package() 
    : m_route_pos(0)
{
}

mp::Package::~Package()
{
    delete m_route_pos;
}

mp::Package::Package(mp::Session &session, const mp::Origin &origin) 
    : m_session(session), m_origin(origin),
      m_route_pos(0)
{
}


mp::Package & mp::Package::copy_filter(const Package &p)
{
    delete m_route_pos;
    m_route_pos = p.m_route_pos->clone();
    return *this;
}


void mp::Package::move()
{
    if (m_route_pos)
    {
        const filter::Base *next_filter = m_route_pos->move(0);
        if (next_filter)
            next_filter->process(*this);
    }
}

void mp::Package::move(std::string route)
{
    if (m_route_pos)
    {
        const char *r_cstr = route.length() ? route.c_str() : 0;
        const filter::Base *next_filter = m_route_pos->move(r_cstr);
        if (next_filter)
            next_filter->process(*this);
    }
}


mp::Session & mp::Package::session()
{
    return m_session;
}

mp::Origin mp::Package::origin() const 
{
    return m_origin;
}
        
mp::Origin & mp::Package::origin()
{
    return m_origin;
}

mp::Package & mp::Package::origin(const Origin & origin)
{
    m_origin = origin;
    return *this;
}

mp::Package & mp::Package::router(const mp::Router &router)
{
    m_route_pos = router.createpos();
    return *this;
}

yazpp_1::GDU &mp::Package::request()
{
    return m_request_gdu;
}


yazpp_1::GDU &mp::Package::response()
{
    return m_response_gdu;
}

mp::Session mp::Package::session() const
{
    return m_session;
}

std::ostream& std::operator<<(std::ostream& os,  mp::Package& p)
{
    os << p.origin() << " ";
    os << p.session().id();
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

