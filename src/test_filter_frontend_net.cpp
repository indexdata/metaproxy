/* $Id: test_filter_frontend_net.cpp,v 1.15 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "util.hpp"
#include "filter_frontend_net.hpp"

#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

class FilterInit: public mp::filter::Base {
public:
    void process(mp::Package & package) const {
        
        if (package.session().is_closed())
        {
            // std::cout << "Got Close.\n";
        }
       
        Z_GDU *gdu = package.request().get();
        if (gdu)
        {
            // std::cout << "Got PDU. Sending init response\n";
            mp::odr odr;
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
            
            apdu->u.initResponse->implementationName = "YP2/YAZ";
            
            package.response() = apdu;
        }
        return package.move();
    };
};


BOOST_AUTO_UNIT_TEST( test_filter_frontend_net_1 )
{
    try 
    {
        {
            mp::filter::FrontendNet nf;
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_UNIT_TEST( test_filter_frontend_net_2 )
{
    try 
    {
        {
	    mp::RouterChain router;

            FilterInit tf;

	    router.append(tf);

            // Create package with Z39.50 init request in it
	    mp::Package pack;

            mp::odr odr;
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_initRequest);
            
            pack.request() = apdu;
	    // Done creating query. 

            // Put it in router
	    pack.router(router).move(); 

            // Inspect that we got Z39.50 init response
            yazpp_1::GDU *gdu = &pack.response();

            Z_GDU *z_gdu = gdu->get();
            BOOST_CHECK(z_gdu);
            if (z_gdu) {
                BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
                BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_initResponse);
            }
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_UNIT_TEST( test_filter_frontend_net_3 )
{
    try 
    {
        {
	    mp::RouterChain router;

            // put in frontend first
            mp::filter::FrontendNet filter_front;

            std::vector <std::string> ports;
            ports.insert(ports.begin(), "unix:socket");
            filter_front.ports() = ports;
            filter_front.listen_duration() = 1;  // listen a short time only
	    router.append(filter_front);

            // put in a backend
            FilterInit filter_init;
	    router.append(filter_init);

	    mp::Package pack;
	    
	    pack.router(router).move(); 
        }
        BOOST_CHECK(true);
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
