
#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "router.hpp"
#include "session.hpp"
#include "package.hpp"
#include "filter_frontend_net.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class FilterInit: public yp2::Filter {
public:
    yp2::Package & process(yp2::Package & package) const {
        ODR odr = odr_createmem(ODR_ENCODE);
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);

        apdu->u.initResponse->implementationName = "YP2/YAZ";

        package.response() = apdu;
        odr_destroy(odr);
	return package.move();
    };
};


BOOST_AUTO_TEST_CASE( test_filter_frontend_net_1 )
{
    try 
    {
        {
            yp2::FilterFrontendNet nf;
        }
        BOOST_CHECK(true);
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

            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack_in(session, origin);
	    
	    pack_in.router(router).move(); 

            yazpp_1::GDU *gdu = &pack_in.response();

            BOOST_CHECK_EQUAL(gdu->get()->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(gdu->get()->u.z3950->which, Z_APDU_initResponse);
        }
        BOOST_CHECK(true);
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

            yp2::FilterFrontendNet filter_front;
            filter_front.listen_address() = "unix:socket";
            filter_front.listen_duration() = 2;  // listen a short time only
	    router.rule(filter_front);

            FilterInit filter_init;
	    router.rule(filter_init);

            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack_in(session, origin);
	    
	    pack_in.router(router).move(); 
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
