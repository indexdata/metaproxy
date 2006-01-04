/* $Id: package.cpp,v 1.2 2006-01-04 11:19:04 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "package.hpp"

yp2::Package::Package() 
    :  m_filter(0), m_router(0), m_data(0)
{
}

yp2::Package::Package(yp2::Session &session, yp2::Origin &origin) 
    : m_session(session), m_origin(origin),
      m_filter(0), m_router(0), m_data(0)
{
}

yp2::Package & yp2::Package::copy_filter(const Package &p)
{
    m_router = p.m_router;
    m_filter = p.m_filter;
    return *this;
}


void yp2::Package::move()
{
    m_filter = m_router->move(m_filter, this);
    if (m_filter)
                m_filter->process(*this);
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
    m_filter = 0;
    m_router = &router;
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
