/* $Id: test_filter_factory.cpp,v 1.3 2005-10-29 22:23:36 marc Exp $
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
     const std::string type() const{
        return "XFilter";
    };
};


yp2::filter::Base* xfilter_creator(){
    return new XFilter;
}

class YFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {};
    const std::string type() const{
        return "YFilter";
    };
};

yp2::filter::Base* yfilter_creator(){
    return new YFilter;
}



//int main(int argc, char **argv)
BOOST_AUTO_TEST_CASE( test_filter_factory_1 )
{
    try {
        
        yp2::filter::FilterFactory  ffactory;
        
        XFilter xf;
        YFilter yf;

        const std::string xfid = xf.type();
        const std::string yfid = yf.type();
        
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
        
        yp2::filter::Base* xfilter = ffactory.create(xfid);
        yp2::filter::Base* yfilter = ffactory.create(yfid);

        BOOST_CHECK_EQUAL(xf.type(), xfilter->type());
        BOOST_CHECK_EQUAL(yf.type(), yfilter->type());

        //std::cout << "Xfilter pointer name:  " << xfilter->type() << std::endl;
        //std::cout << "Yfilter pointer name:  " << yfilter->type() << std::endl;
        

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
