/* $Id: filter_query_rewrite.cpp,v 1.1 2006-01-19 12:18:09 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"

//#include <boost/thread/mutex.hpp>

#include "util.hpp"
#include "filter_query_rewrite.hpp"

#include <yaz/zgdu.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class QueryRewrite::Rep {
            friend class QueryRewrite;
            int dummy;
        };
    }
}

yf::QueryRewrite::QueryRewrite() : m_p(new Rep)
{
    m_p->dummy = 1;
}

yf::QueryRewrite::~QueryRewrite()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::QueryRewrite::process(yp2::Package &package) const
{

    if (package.session().is_closed())
    {
        std::cout << "Got Close.\n";
    }
    
    Z_GDU *gdu = package.request().get();
    
    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_initRequest)
    {
        std::cout << "Got Z3950 Init PDU\n";         
        //Z_InitRequest *req = gdu->u.z3950->u.initRequest;
        //package.request() = gdu;
    } 
    else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
             Z_APDU_searchRequest)
    {
        std::cout << "Got Z3950 Search PDU\n";   
        //Z_SearchRequest *req = gdu->u.z3950->u.searchRequest;
        //package.request() = gdu;
    } 
    else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
             Z_APDU_scanRequest)
    {
        std::cout << "Got Z3950 Scan PDU\n";   
        //Z_ScanRequest *req = gdu->u.z3950->u.scanRequest;
        //package.request() = gdu;
    } 
    package.move();
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::QueryRewrite;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_query_rewrite = {
        0,
        "query-rewrite",
        filter_creator
    };
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_query_rewrite;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
