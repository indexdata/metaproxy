

#include "config.hpp"
#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <iostream>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;


class FilterConstant: public yp2::Filter {
public:
    yp2::Package & process(yp2::Package & package) const {
	package.data() = 1234;
	return package.move();
    };
};


class FilterDouble: public yp2::Filter {
public:
    yp2::Package & process(yp2::Package & package) const {
	package.data() = package.data() * 2;
	return package.move();
    };
};
    
    
BOOST_AUTO_TEST_CASE( testfilter2 ) 
{
    try {
	FilterConstant fc;
        fc.name() = "FilterConstant";
	FilterDouble fd;
        fd.name() = "FilterDouble";

	{
	    yp2::RouterChain router1;
	    
	    // test filter set/get/exception
	    router1.rule(fc);
	    
	    router1.rule(fd);

            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack_in(session, origin);
	    
	    yp2::Package pack_out = pack_in.router(router1).move(); 
	    
            BOOST_CHECK (pack_out.data() == 2468);
            
        }
        
        {
	    yp2::RouterChain router2;
	    
	    router2.rule(fd);
	    router2.rule(fc);
	    
            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack_in(session, origin);
	 
	    yp2::Package pack_out(session, origin);

            pack_out = pack_in.router(router2).move();
     
            BOOST_CHECK (pack_out.data() == 1234);
            
	}

    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ...) {
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
