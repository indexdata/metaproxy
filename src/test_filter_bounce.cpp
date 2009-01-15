/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

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

#include "config.hpp"
#include "filter_bounce.hpp"
#include "util.hpp"
#include "gduutil.hpp"
//#include "sru_util.hpp"
#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

#include <iostream>
#include <stdexcept>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>




using namespace boost::unit_test;

namespace mp = metaproxy_1;
using namespace mp::util;

void check_bounce_z3950(mp::RouterChain &router, int request, int response)
{

    bool print = false;
    if (print)
        std::cout << "check_bounce_z3950\n";

    // Create package with Z39.50 init request in it
    mp::Package pack;
        
    mp::odr odr;
    Z_APDU *apdu = zget_APDU(odr, request);

    pack.request() = apdu;

    // Put it in router
    pack.router(router).move(); 
    
    // Inspect bounced back request
    yazpp_1::GDU *gdu_req = &pack.request();
    yazpp_1::GDU *gdu_res = &pack.response();
    
    Z_GDU *z_gdu_req = gdu_req->get();
    Z_GDU *z_gdu_res = gdu_res->get();

    BOOST_CHECK(z_gdu_req);
    if (z_gdu_req) {
        if (print)
            std::cout << "Z_GDU " << *(z_gdu_req) << "\n";
        BOOST_CHECK_EQUAL(z_gdu_req->which, Z_GDU_Z3950);
        BOOST_CHECK_EQUAL(z_gdu_req->u.z3950->which, request);
    }

    BOOST_CHECK(z_gdu_res);
    if (z_gdu_res) {
        if (print)
            std::cout << "Z_GDU " << *(z_gdu_res) << "\n";
        BOOST_CHECK_EQUAL(z_gdu_res->which, Z_GDU_Z3950);
        BOOST_CHECK_EQUAL(z_gdu_res->u.z3950->which, response);
    }
}

void check_bounce_http(mp::RouterChain &router)
{

    bool print = false;
    if (print)
        std::cout << "check_bounce_http\n";

    // Create package with Z39.50 init request in it
    mp::Package pack;
        
    mp::odr odr;
    Z_GDU *gdu = z_get_HTTP_Request(odr);
    //z_get_HTTP_Request_host_path(odr, host, path);
    pack.request() = gdu;

    // Put it in router
    pack.router(router).move(); 
    
    // Inspect bounced back request
    yazpp_1::GDU *gdu_req = &pack.request();
    yazpp_1::GDU *gdu_res = &pack.response();
    
    Z_GDU *z_gdu_req = gdu_req->get();
    Z_GDU *z_gdu_res = gdu_res->get();

    BOOST_CHECK(z_gdu_req);
    if (z_gdu_req) {
        if (print)
            std::cout << "Z_GDU " << *(z_gdu_req) << "\n";
        BOOST_CHECK_EQUAL(z_gdu_req->which, Z_GDU_HTTP_Request);
    }

    BOOST_CHECK(z_gdu_res);
    if (z_gdu_res) {
        if (print)
            std::cout << "Z_GDU " << *(z_gdu_res) << "\n";
        BOOST_CHECK_EQUAL(z_gdu_res->which,  Z_GDU_HTTP_Response);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_bounce_1 )
{
    try 
    {
        mp::filter::Bounce f_bounce;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_bounce_2 )
{
    try 
    {
        mp::RouterChain router;        
        mp::filter::Bounce f_bounce;
        router.append(f_bounce);

        check_bounce_z3950(router, 
                           Z_APDU_initRequest, Z_APDU_close);
        //check_bounce_z3950(router, 
        //                   Z_APDU_searchRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_presentRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_deleteResultSetRequest, Z_APDU_close);
        //check_bounce_z3950(router, 
        //                   Z_APDU_accessControlRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_resourceControlRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_triggerResourceControlRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_resourceReportRequest, Z_APDU_close);
        //check_bounce_z3950(router, 
        //                   Z_APDU_scanRequest, Z_APDU_close);
        //check_bounce_z3950(router, 
        //                   Z_APDU_sortRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_segmentRequest, Z_APDU_close);
        //check_bounce_z3950(router, 
        //                   Z_APDU_extendedServicesRequest, Z_APDU_close);
        check_bounce_z3950(router, 
                           Z_APDU_close , Z_APDU_close);
        //check_bounce_z3950(router, 
        //                   Z_APDU_duplicateDetectionRequest, Z_APDU_close);


    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_bounce_3 )
{
    try 
    {
        mp::RouterChain router;        
        mp::filter::Bounce f_bounce;
        router.append(f_bounce);

        check_bounce_http(router);

    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

