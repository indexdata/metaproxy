/* $Id: test_filter_factory.cpp,v 1.10 2006-01-04 14:30:51 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%

*/

#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"
#include "factory_filter.hpp"


#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

// XFilter
class XFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const;
};

void XFilter::process(yp2::Package & package) const
{
    package.data() = 1;
}

static yp2::filter::Base* xfilter_creator(){
    return new XFilter;
}

// YFilter ...
class YFilter: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const;
};

void YFilter::process(yp2::Package & package) const
{
    package.data() = 2;
}

static yp2::filter::Base* yfilter_creator(){
    return new YFilter;
}

BOOST_AUTO_UNIT_TEST( test_filter_factory_1 )
{
    try {
        
        yp2::FactoryFilter  ffactory;
        
        XFilter xf;
        YFilter yf;

        const std::string xfid = "XFilter";
        const std::string yfid = "YFilter";
        
        BOOST_CHECK(ffactory.add_creator(xfid, xfilter_creator));
        BOOST_CHECK(ffactory.drop_creator(xfid));
        BOOST_CHECK(ffactory.add_creator(xfid, xfilter_creator));
        BOOST_CHECK(ffactory.add_creator(yfid, yfilter_creator));
        
        yp2::filter::Base* xfilter = 0;
        xfilter = ffactory.create(xfid);
        yp2::filter::Base* yfilter = 0;
        yfilter = ffactory.create(yfid);

        BOOST_CHECK(0 != xfilter);
        BOOST_CHECK(0 != yfilter);

        yp2::Package pack;
        xfilter->process(pack);
        BOOST_CHECK_EQUAL(pack.data(), 1);

        yfilter->process(pack);
        BOOST_CHECK_EQUAL(pack.data(), 2);
    }
    catch ( ... ) {
        throw;
        BOOST_CHECK (false);
    }
}

#if HAVE_DL_SUPPORT
#if HAVE_DLFCN_H
BOOST_AUTO_UNIT_TEST( test_filter_factory_2 )
{
    try {        
        yp2::FactoryFilter  ffactory;
        
        const std::string id = "dl";
        
        // first load
        BOOST_CHECK(ffactory.add_creator_dyn(id, ".libs"));

        // test double load
        BOOST_CHECK(ffactory.add_creator_dyn(id, ".libs"));
                
        yp2::filter::Base* filter = 0;
        filter = ffactory.create(id);

        BOOST_CHECK(0 != filter);

        yp2::Package pack;
        filter->process(pack);
        BOOST_CHECK_EQUAL(pack.data(), 42); // magic from filter_dl ..
    }
    catch ( ... ) {
        throw;
        BOOST_CHECK (false);
    }
}
#endif
#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
