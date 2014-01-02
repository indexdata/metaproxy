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
#include "filter_sru_to_z3950.hpp"
#include <metaproxy/util.hpp>
#include "sru_util.hpp"
#include <metaproxy/router_chain.hpp>
#include <metaproxy/package.hpp>

#include <iostream>
#include <stdexcept>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

namespace mp = metaproxy_1;
using namespace mp::util;



BOOST_AUTO_TEST_CASE( test_filter_sru_to_z3950_1 )
{
    try
    {
        mp::filter::SRUtoZ3950 f_sru_to_z3950;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_sru_to_z3950_2 )
{
    try
    {
        mp::RouterChain router;

        mp::filter::SRUtoZ3950 f_sru_to_z3950;

        router.append(f_sru_to_z3950);

        //check_sru_to_z3950_init(router);
        //check_sru_to_z3950_search(router,
        //                           "@attrset Bib-1 @attr 1=4 the",
        //                           "@attrset Bib-1 @attr 1=4 the");

    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}


// BOOST_AUTO_TEST_CASE( test_filter_sru_to_z3950_3 )
// {


//     try
//     {
//         mp::RouterChain router;


//         std::string xmlconf =
//             "<?xml version='1.0'?>\n"
//             "<filter xmlns='http://indexdata.com/metaproxy'\n"
//             "        id='qrw1' type='sru_to_z3950'>\n"
//             "</filter>\n"
//             ;

//         //std::cout << xmlconf  << std::endl;

//         // reading and parsing XML conf
//         xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(), xmlconf.size());
//         BOOST_CHECK(doc);
//         xmlNode *root_element = xmlDocGetRootElement(doc);

//         // creating and configuring filter
//         mp::filter::SRUtoZ3950 f_sru_to_z3950;
//         f_sru_to_z3950.configure(root_element);

//         // remeber to free XML DOM
//         xmlFreeDoc(doc);

//         // add only filter to router
//         router.append(f_sru_to_z3950);

//         // start testing
//         check_sru_to_z3950_init(router);
//         check_sru_to_z3950_search(router,
//                                    "@attrset Bib-1 @attr 1=4 the",
//                                    "@attrset Bib-1 @attr 1=4 the");

//     }

//     catch (std::exception &e) {
//         std::cout << e.what() << "\n";
//         BOOST_CHECK (false);
//     }

//     catch ( ... ) {
//         BOOST_CHECK (false);
//     }
// }

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

