/* $Id: test_filter_factory.cpp,v 1.5 2005-11-10 23:10:42 adam Exp $
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
};


yp2::filter::Base* xfilter_creator(){
    return new XFilter;
}

class YFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
};

yp2::filter::Base* yfilter_creator(){
    return new YFilter;
}


BOOST_AUTO_TEST_CASE( test_filter_factory_1 )
{
    try {
        
        yp2::FilterFactory  ffactory;
        
        XFilter xf;
        YFilter yf;

        const std::string xfid = "XFilter";
        const std::string yfid = "YFilter";
        
        //std::cout << "Xfilter name: " << xfid << std::endl;
        //std::cout << "Yfilter name: " << yfid << std::endl;

        BOOST_CHECK_EQUAL(ffactory.add_creator(xfid, xfilter_creator),
                          true);
        BOOST_CHECK_EQUAL(ffactory.drop_creator(xfid),
                          true);
        BOOST_CHECK_EQUAL(ffactory.add_creator(xfid, xfilter_creator),
                          true);
        BOOST_CHECK_EQUAL(ffactory.add_creator(yfid, yfilter_creator),
                          true);
        
        yp2::filter::Base* xfilter = 0;
        xfilter = ffactory.create(xfid);
        yp2::filter::Base* yfilter = 0;
        yfilter = ffactory.create(yfid);

        //BOOST_CHECK_EQUAL(sizeof(xf), sizeof(*xfilter));
        //BOOST_CHECK_EQUAL(sizeof(yf), sizeof(*yfilter));

        BOOST_CHECK(0 != xfilter);
        BOOST_CHECK(0 != yfilter);
    }
    catch ( ... ) {
        throw;
        BOOST_CHECK (false);
    }
        
    std::exit(0);
}

// get function - right val in assignment
//std::string name() const {
//return m_name;
//  return "Base";
//}

// set function - left val in assignment
//std::string & name() {
//    return m_name;
//}

// set function - can be chained
//Base & name(const std::string & name){
//  m_name = name;
//  return *this;
//}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
