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

#include "config.hpp"
#include <iostream>
#include <string>
#include <stdexcept>

#include <metaproxy/util.hpp>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

bool test(const char *pattern, const char *value) {
    std::string pattern1(pattern);
    std::string value1(value);
    return mp::util::match_ip(pattern1, value1);
}

BOOST_AUTO_TEST_CASE(match_ip4)
{
    BOOST_CHECK(test("127.0.0.1", "127.0.0.1"));
    BOOST_CHECK(!test("127.0.0.11", "127.0.0.1"));
    BOOST_CHECK(!test("127.0.0.1", "127.0.0.11"));
    BOOST_CHECK(test("127.0.*", "127.0.0.1"));
    BOOST_CHECK(test("127.*.1", "127.0.0.1"));
    BOOST_CHECK(test("127.*.*.1", "127.0.0.1"));
    BOOST_CHECK(!test("127.*.*.1", "127.0.1"));
    BOOST_CHECK(!test("127.*.*.1", "127.0.1"));
}

BOOST_AUTO_TEST_CASE(match_ip4_mapped)
{
    BOOST_CHECK(test("127.0.0.1", "::ffff:127.0.0.1"));
    BOOST_CHECK(!test("127.0.0.11", "::ffff:127.0.0.1"));
    BOOST_CHECK(!test("127.0.0.1", "::ffff:127.0.0.11"));
    BOOST_CHECK(test("127.0.*", "::ffff:127.0.0.1"));
    BOOST_CHECK(test("127.*.1", "::ffff:127.0.0.1"));
    BOOST_CHECK(test("127.*.*.1", "::ffff:127.0.0.1"));
    BOOST_CHECK(!test("127.*.*.1", "::ffff:127.0.1"));
    BOOST_CHECK(!test("127.*.*.1", "::ffff:127.0.1"));
}

BOOST_AUTO_TEST_CASE(match_ip6)
{
    BOOST_CHECK(test("::1", "::1"));
    BOOST_CHECK(!test("::1", "::ffff:::1"));
    BOOST_CHECK(test("fe80::a161:aae:4fe0:fed9", "fe80::a161:aae:4fe0:fed9"));
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

