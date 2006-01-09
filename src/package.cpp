/* $Id: package.cpp,v 1.4 2006-01-09 13:43:59 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "package.hpp"

yp2::Package::Package() 
    :  
#if ROUTE_POS
    m_route_pos(0),
#else
    m_filter(0), m_router(0),
#endif
    m_data(0)
{
}

yp2::Package::~Package()
{
#if ROUTE_POS
    delete m_route_pos;
#endif
}

yp2::Package::Package(yp2::Session &session, yp2::Origin &origin) 
    : m_session(session), m_origin(origin),
#if ROUTE_POS
      m_route_pos(0),
#else
      m_filter(0), m_router(0),
#endif
      m_data(0)
{
}

yp2::Package & yp2::Package::copy_filter(const Package &p)
{
#if ROUTE_POS
    m_route_pos = p.m_route_pos->clone();
#else
    m_router = p.m_router;
    m_filter = p.m_filter;
#endif
    return *this;
}


void yp2::Package::move()
{
#if ROUTE_POS
    if (m_route_pos)
    {
        const filter::Base *next_filter = m_route_pos->move();
        if (next_filter)
            next_filter->process(*this);
    }
#else
    m_filter = m_router->move(m_filter, this);
    if (m_filter)
        m_filter->process(*this);
#endif
}

yp2::Session & yp2::Package::session()
{
    return m_session;
}


int yp2::Package::data() const
{
    return m_data;
}

int & yp2::Package::data()
{
    return m_data;
}
        
yp2::Package & yp2::Package::data(const int & data)
{
    m_data = data;
    return *this;
}

yp2::Origin yp2::Package::origin() const 
{
    return m_origin;
}
        
yp2::Origin & yp2::Package::origin()
{
    return m_origin;
}

yp2::Package & yp2::Package::origin(const Origin & origin)
{
    m_origin = origin;
    return *this;
}

yp2::Package & yp2::Package::router(const yp2::Router &router)
{
#if ROUTE_POS
    m_route_pos = router.createpos();
#else
    m_filter = 0;
    m_router = &router;
#endif
    return *this;
}

yazpp_1::GDU &yp2::Package::request()
{
    return m_request_gdu;
}


yazpp_1::GDU &yp2::Package::response()
{
    return m_response_gdu;
}

yp2::Session yp2::Package::session() const
{
    return m_session;
}
                
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
