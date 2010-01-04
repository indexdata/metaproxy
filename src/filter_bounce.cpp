/* This file is part of Metaproxy.
   Copyright (C) 2005-2010 Index Data

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

#include "filter_bounce.hpp"
#include "package.hpp"
#include "util.hpp"
#include "gduutil.hpp"

#include <yaz/zgdu.h>

#include <sstream>

//#include "config.hpp"
//#include "filter.hpp"

//#include <boost/thread/mutex.hpp>





namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Bounce::Rep {
            friend class Bounce;
            bool bounce;
        };
    }
}

yf::Bounce::Bounce() : m_p(new Rep)
{
    m_p->bounce = true;
}

yf::Bounce::~Bounce()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Bounce::process(mp::Package &package) const
{
    if (! m_p->bounce )
    {
        package.move();
        return;
    }
    package.session().close();
    
    Z_GDU *zgdu = package.request().get();
    
    if (!zgdu)
        return;
    
    //std::string message("BOUNCE ");
    std::ostringstream message;    
    message << "BOUNCE " << *zgdu;
    
    metaproxy_1::odr odr; 
    
    if (zgdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu_res = 0;
        apdu_res = odr.create_close(zgdu->u.z3950,
                                    Z_Close_systemProblem,
                                    message.str().c_str());
        package.response() = apdu_res;
    }
    else if (zgdu->which == Z_GDU_HTTP_Request)
    {
        Z_GDU *zgdu_res = 0;
        zgdu_res 
            = odr.create_HTTP_Response(package.session(), 
                                       zgdu->u.HTTP_Request, 400);
        
        package.response() = zgdu_res;
    }
    else if (zgdu->which == Z_GDU_HTTP_Response)
    {
    }
    

    return;
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::Bounce;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_bounce = {
        0,
        "bounce",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

