/* $Id: package.cpp,v 1.12 2006-08-30 08:44:58 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */


#include "config.hpp"
#include "package.hpp"

#include <iostream>

namespace mp = metaproxy_1;

mp::Package::Package() 
    : m_route_pos(0), m_data(0)
{
}

mp::Package::~Package()
{
    delete m_route_pos;
}

mp::Package::Package(mp::Session &session, const mp::Origin &origin) 
    : m_session(session), m_origin(origin),
      m_route_pos(0), m_data(0)
{
}

mp::Package::Package(mp::Session &session,
                     const mp::Origin &origin, const mp::Origin &target) 
    : m_session(session), m_origin(origin), m_target(target),
      m_route_pos(0), m_data(0)
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


int mp::Package::data() const
{
    return m_data;
}

int & mp::Package::data()
{
    return m_data;
}
        
mp::Package & mp::Package::data(const int & data)
{
    m_data = data;
    return *this;
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

mp::Origin mp::Package::target() const 
{
    return m_target;
}
        
mp::Origin & mp::Package::target()
{
    return m_target;
}

mp::Package & mp::Package::target(const Origin & target)
{
    m_target = target;
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
    os << p.session().id() << " ";
    os << p.origin();
    return os;
}

std::ostream& std::operator<<(std::ostream& os,  mp::Origin& o)
{
    if (o.address != "")
        os << o.address;
    else
        os << "0";
    os << ":" << o.port;
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
