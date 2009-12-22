/* This file is part of Metaproxy.
   Copyright (C) 2005-2009 Index Data

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
#include <iostream>
#include <stdexcept>

#include "util.hpp"
#include "filter_frontend_net.hpp"

#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
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
        if (gdu && gdu->which == Z_GDU_Z3950)
        {
            // std::cout << "Got PDU. Sending init response\n";
            mp::odr odr;
            Z_APDU *apdu = odr.create_initResponse(gdu->u.z3950, 0, 0);
            package.response() = apdu;
        }
        return package.move();
    };
};


BOOST_AUTO_TEST_CASE( test_filter_frontend_net_1 )
{
    try 
    {
        {
            mp::filter::FrontendNet nf;
        }
    }
    catch (std::runtime_error &e) {
        std::cerr << "std::runtime error: " << e.what() << std::endl;
        BOOST_CHECK(false);
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
    catch (std::runtime_error &e) {
        std::cerr << "std::runtime error: " << e.what() << std::endl;
        BOOST_CHECK(false);
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
	    mp::RouterChain router;

            // put in frontend first
            mp::filter::FrontendNet filter_front;

            std::vector <std::string> ports;
            ports.insert(ports.begin(), "unix:socket");
            filter_front.set_ports(ports);
            filter_front.set_listen_duration(1);  // listen a short time only
	    router.append(filter_front);

            // put in a backend
            FilterInit filter_init;
	    router.append(filter_init);

	    mp::Package pack;
	    
	    pack.router(router).move(); 
        }
        BOOST_CHECK(true);
    }
    catch (std::runtime_error &e) {
        std::cerr << "std::runtime error: " << e.what() << std::endl;
        BOOST_CHECK(false);
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

