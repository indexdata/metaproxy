/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include "factory_filter.hpp"


#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

// XFilter
class XFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const;
    void configure(const xmlNode* ptr, bool test_only, const char *path) {};
};

void XFilter::process(mp::Package & package) const
{
    //package.data() = 1;
}

static mp::filter::Base* xfilter_creator(){
    return new XFilter;
}

// YFilter ...
class YFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const;
    void configure(const xmlNode* ptr, bool test_only, const char *path) {};
};

void YFilter::process(mp::Package & package) const
{
    //package.data() = 2;
}

static mp::filter::Base* yfilter_creator(){
    return new YFilter;
}

BOOST_AUTO_TEST_CASE( test_filter_factory_1 )
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
        //BOOST_CHECK_EQUAL(pack.data(), 1);

        yfilter->process(pack);
        //BOOST_CHECK_EQUAL(pack.data(), 2);
    }
    catch ( ... ) {
        throw;
        BOOST_CHECK (false);
    }
}

#if HAVE_DL_SUPPORT
#if HAVE_DLFCN_H
BOOST_AUTO_TEST_CASE( test_filter_factory_2 )
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
        //BOOST_CHECK_EQUAL(pack.data(), 42); // magic from filter_dl ..
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

