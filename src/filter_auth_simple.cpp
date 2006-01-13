/* $Id: filter_auth_simple.cpp,v 1.2 2006-01-13 15:09:35 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "util.hpp"
#include "filter_auth_simple.hpp"

#include <yaz/zgdu.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class AuthSimple::Rep {
            friend class AuthSimple;
            int dummy; // private data
        };
    }
}

yf::AuthSimple::AuthSimple() : m_p(new Rep)
{
    m_p->dummy = 1;
}

yf::AuthSimple::~AuthSimple()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::AuthSimple::process(yp2::Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_initRequest)
    {
        // we have a Z39.50 init request
        Z_InitRequest *init = gdu->u.z3950->u.initRequest;

        // for now reject if we don't supply _some_ auth
        if (!init->idAuthentication)
        {
            yp2::odr odr;
            Z_APDU *apdu = odr.create_initResponse(gdu->u.z3950, 0, 0);
            
            apdu->u.initResponse->implementationName = "YP2/YAZ";
            *apdu->u.initResponse->result = 0; // reject
            
            package.response() = apdu;

            package.session().close();
            return;
        }
        // if we get here access is granted..

        // should authentication be altered of deleted?
        // that could be configurable..
    }
    package.move();  // pass on package
}

void yp2::filter::AuthSimple::configure(const xmlNode * ptr)
{
    // Read XML config.. Put config info in m_p..
    m_p->dummy = 1;  
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::AuthSimple;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_auth_simple = {
        0,
        "auth_simple",
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
