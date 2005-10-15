
#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter_frontend_net.hpp"

#include "router.hpp"
#include "session.hpp"
#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class FilterInit: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {

        if (package.session().is_closed())
        {
            // std::cout << "Got Close.\n";
        }
       
        Z_GDU *gdu = package.request().get();
        if (gdu)
        {
            // std::cout << "Got PDU. Sending init response\n";
            ODR odr = odr_createmem(ODR_ENCODE);
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
            
            apdu->u.initResponse->implementationName = "YP2/YAZ";
            
            package.response() = apdu;
            odr_destroy(odr);
        }
        return package.move();
    };
};


BOOST_AUTO_TEST_CASE( test_filter_frontend_net_1 )
{
    try 
    {
        {
            yp2::filter::FrontendNet nf;
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_frontend_net_2 )
{
    try 
    {
        {
	    yp2::RouterChain router;

            FilterInit tf;

	    router.rule(tf);

            // Create package with Z39.50 init request in it
	    yp2::Package pack;

            ODR odr = odr_createmem(ODR_ENCODE);
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_initRequest);
            
            pack.request() = apdu;
            odr_destroy(odr);
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

BOOST_AUTO_TEST_CASE( test_filter_frontend_net_3 )
{
    try 
    {
        {
	    yp2::RouterChain router;

            // put in frontend first
            yp2::filter::FrontendNet filter_front;

            std::vector <std::string> ports;
            ports.insert(ports.begin(), "unix:socket");
            filter_front.ports() = ports;
            filter_front.listen_duration() = 1;  // listen a short time only
	    router.rule(filter_front);

            // put in a backend
            FilterInit filter_init;
	    router.rule(filter_init);

	    yp2::Package pack;
	    
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
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
