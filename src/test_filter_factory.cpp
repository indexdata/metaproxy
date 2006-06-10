/* $Id: test_filter_factory.cpp,v 1.13 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
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
namespace mp = metaproxy_1;

// XFilter
class XFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const;
};

void XFilter::process(mp::Package & package) const
{
    package.data() = 1;
}

static mp::filter::Base* xfilter_creator(){
    return new XFilter;
}

// YFilter ...
class YFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const;
};

void YFilter::process(mp::Package & package) const
{
    package.data() = 2;
}

static mp::filter::Base* yfilter_creator(){
    return new YFilter;
}

BOOST_AUTO_UNIT_TEST( test_filter_factory_1 )
{
    try {
        
        mp::FactoryFilter  ffactory;
        
        XFilter xf;
        YFilter yf;

        const std::string xfid = "XFilter";
        const std::string yfid = "YFilter";
        
        BOOST_CHECK(ffactory.add_creator(xfid, xfilter_creator));
        BOOST_CHECK(ffactory.drop_creator(xfid));
        BOOST_CHECK(ffactory.add_creator(xfid, xfilter_creator));
        BOOST_CHECK(ffactory.add_creator(yfid, yfilter_creator));
        
        mp::filter::Base* xfilter = 0;
        xfilter = ffactory.create(xfid);
        mp::filter::Base* yfilter = 0;
        yfilter = ffactory.create(yfid);

        BOOST_CHECK(0 != xfilter);
        BOOST_CHECK(0 != yfilter);

        mp::Package pack;
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
        mp::FactoryFilter  ffactory;
        
        const std::string id = "dl";
        
        // first load
        BOOST_CHECK(ffactory.add_creator_dl(id, ".libs"));

        // test double load
        BOOST_CHECK(ffactory.add_creator_dl(id, ".libs"));
                
        mp::filter::Base* filter = 0;
        filter = ffactory.create(id);

        BOOST_CHECK(0 != filter);

        mp::Package pack;
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
