/* $Id: origin.cpp,v 1.1 2006-08-30 10:48:52 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */


#include "config.hpp"
#include "package.hpp"

#include <iostream>

namespace mp = metaproxy_1;

mp::Origin::Origin() : type(API) 
{ 
    origin_id = 0; 
}

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
