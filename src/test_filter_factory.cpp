/* $Id: test_filter_factory.cpp,v 1.2 2005-10-29 17:58:14 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"
#include "filter_factory.hpp"


#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class XFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
    std::string name(){
        return std::string("xfilter");
        }   
};


yp2::filter::Base* xfilter_creator(){
    return new XFilter;
}


class YFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
    std::string name(){
        return std::string("yfilter");
        }   
};

yp2::filter::Base* yfilter_creator(){
    return new YFilter;
}



//int main(int argc, char **argv)
BOOST_AUTO_TEST_CASE( test_filter_factory_1 )
{
    try {
        
        yp2::filter::FilterFactory  ffactory;
        
        BOOST_CHECK_EQUAL(ffactory.add_creator("xfilter", xfilter_creator),
                          true);
        BOOST_CHECK_EQUAL(ffactory.drop_creator("xfilter"),
                          true);
        BOOST_CHECK_EQUAL(ffactory.add_creator("xfilter", xfilter_creator),
                          true);
        BOOST_CHECK_EQUAL(ffactory.add_creator("yfilter", yfilter_creator),
                          true);
        
        yp2::filter::Base* xfilter = ffactory.create("xfilter");
        yp2::filter::Base* yfilter = ffactory.create("yfilter");
        
        //BOOST_CHECK_EQUAL(xfilter->name(), std::string("xfilter"));
        //BOOST_CHECK_EQUAL(yfilter->name(), std::string("yfilter"));
        
        }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
        
    std::exit(0);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
