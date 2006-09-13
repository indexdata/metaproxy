/* $Id: filter_sru_to_z3950.cpp,v 1.1 2006-09-13 10:43:24 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "util.hpp"
#include "filter_sru_to_z3950.hpp"

#include <yaz/zgdu.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class SRUtoZ3950::Rep {
            friend class SRUtoZ3950;
            //int dummy;
        };
    }
}

yf::SRUtoZ3950::SRUtoZ3950() : m_p(new Rep)
{
    //m_p->dummy = 1;
}

yf::SRUtoZ3950::~SRUtoZ3950()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::SRUtoZ3950::process(mp::Package &package) const
{
    Z_GDU *zgdu_req = package.request().get();

    // ignoring all non HTTP_Request packages
    if (!zgdu_req || !(zgdu_req->which == Z_GDU_HTTP_Request)){
        package.move();
        return;
    }
    
    // only working on  HTTP_Request packages now
    Z_HTTP_Request* http_req =  zgdu_req->u.HTTP_Request;

    // TODO: SRU package checking and translation to Z3950 package

    // sending Z3950 package through pipeline
    package.move();


    // TODO: Z3950 response parsing and translation to SRU package
    //Z_HTTP_Response* http_res = 0;


    Z_GDU *zgdu_res = 0; 
    metaproxy_1::odr odr; 
    zgdu_res 
       = odr.create_HTTP_Response(package.session(), 
                                  zgdu_req->u.HTTP_Request, 200);

    //zgdu_res->u.HTTP_Response->content_len = message.str().size();
    //zgdu_res->u.HTTP_Response->content_buf 
    //    = (char*) odr_malloc(odr, zgdu_res->u.HTTP_Response->content_len);

    //strncpy(zgdu_res->u.HTTP_Response->content_buf, 
    //        message.str().c_str(),  zgdu_res->u.HTTP_Response->content_len);
    
        // z_HTTP_header_add(o, &hres->headers,
        //            "Content-Type", content_type.c_str());
    package.response() = zgdu_res;

}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::SRUtoZ3950;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_sru_to_z3950 = {
        0,
        "SRUtoZ3950",
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
