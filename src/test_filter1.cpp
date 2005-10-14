
#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class TFilter: public yp2::Filter {
public:
    void process(yp2::Package & package) const {};
};
    

BOOST_AUTO_TEST_CASE( test_filter1 )
{
    try{
        TFilter filter;
        
        filter.name("filter1");
        
        BOOST_CHECK (filter.name() == "filter1");
        
        filter.name() = "filter1 rename";
        
        BOOST_CHECK(filter.name() == "filter1 rename");
    }
    //catch(std::runtime_error &e ){
    //    BOOST_CHECK (true);
    //}
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
