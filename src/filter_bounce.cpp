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

/* filter_bounce
A very simple filter that produces some response, in case no earlier 
filter has done so, a kind of last resort fallback. Also supports dumping
the request in that response, for debugging and testing purposes
*/

#include "filter_bounce.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include "gduutil.hpp"

#include <yaz/zgdu.h>

#include <sstream>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Bounce::Rep {
            friend class Bounce;
            bool echo;  // indicates that we wish to echo the request in the 
                        // HTTP response
        };
    }
}

yf::Bounce::Bounce() : m_p(new Rep)
{
    m_p->echo = false;
}

yf::Bounce::~Bounce()
{  // must have a destructor because of boost::scoped_ptr to m_p
}


// Dump the http request into the content of the http response
static void http_echo(mp::odr &odr, Z_GDU *zgdu, Z_GDU *zgdu_res)
{
    int len;
    ODR enc = odr_createmem(ODR_ENCODE);
    //int r =
    (void) z_GDU(enc, &zgdu, 0, 0);
    char *buf = odr_getbuf(enc, &len, 0);
    //h.db( "\n" + msg + "\n" + std::string(buf,len) );
    Z_HTTP_Response *hres = zgdu_res->u.HTTP_Response;
    if (hres)
    {
        z_HTTP_header_set(odr, &hres->headers,
                          "Content-Type", "text/plain");
        
        hres->content_buf = (char*) odr_malloc(odr, len);
        memcpy(hres->content_buf, buf, len);
        hres->content_len = len;        
    }
    odr_destroy(enc);
    
}


void yf::Bounce::process(mp::Package &package) const
{
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
        // TODO - Some day we may want a dump of the request in some
        // addinfo in the close response
        package.response() = apdu_res;
    }
    else if (zgdu->which == Z_GDU_HTTP_Request)
    {
        Z_GDU *zgdu_res = 0;
        zgdu_res
            = odr.create_HTTP_Response(package.session(),
                                       zgdu->u.HTTP_Request, 400);
        if (m_p->echo) 
        {
            http_echo(odr, zgdu, zgdu_res);
        }
        package.response() = zgdu_res;
    }
    else if (zgdu->which == Z_GDU_HTTP_Response)
    {
    }


    return;
}

void mp::filter::Bounce::configure(const xmlNode * ptr, bool test_only,
                                   const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        else if (!strcmp((const char *) ptr->name, "echo"))
        {
            m_p->echo = mp::xml::get_bool(ptr, 0);
        }
        else
        {
            throw mp::filter::FilterException
            ("Bad element '"
            + std::string((const char *) ptr->name)
            + "' in bounce filter");
        }
    }
    
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

