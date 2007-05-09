/* $Id: filter_bounce.cpp,v 1.4 2007-05-09 21:23:09 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
