/* $Id: origin.cpp,v 1.2 2006-09-26 11:37:08 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */


#include "config.hpp"
#include "package.hpp"

#include <iostream>

namespace mp = metaproxy_1;

mp::Origin::Origin(std::string server_host, 
                   unsigned int server_port) 
    : type(API), address(""), origin_id(0),
      m_server_host(server_host), m_server_port(server_port)
{
}

std::string mp::Origin::server_host() const
{
    return m_server_host;
};

unsigned int mp::Origin::server_port() const
{
    return m_server_port;
};


void mp::Origin::set_tcpip_address(std::string addr, unsigned long s)
{
    address = addr;
    origin_id = s;
    type = TCPIP;
}

std::ostream& std::operator<<(std::ostream& os,  mp::Origin& o)
{
    if (o.address != "")
        os << o.address;
    else
        os << "0";
    os << ":" << o.origin_id;
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
