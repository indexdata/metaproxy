/* $Id: origin.cpp,v 1.5 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
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
