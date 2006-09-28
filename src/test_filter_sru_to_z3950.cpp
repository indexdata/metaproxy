/* $Id: test_filter_sru_to_z3950.cpp,v 1.2 2006-09-28 11:56:54 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter_sru_to_z3950.hpp"
#include "util.hpp"
#include "sru_util.hpp"
#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

#include <iostream>
#include <stdexcept>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

namespace mp = metaproxy_1;
using namespace mp::util;



BOOST_AUTO_UNIT_TEST( test_filter_sru_to_z3950_1 )
{
    try 
    {
        mp::filter::SRUtoZ3950 f_sru_to_z3950;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_UNIT_TEST( test_filter_sru_to_z3950_2 )
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


// BOOST_AUTO_UNIT_TEST( test_filter_sru_to_z3950_3 )
// {
    

//     try 
//     {
//         mp::RouterChain router;
        

//         std::string xmlconf = 
//             "<?xml version='1.0'?>\n"
//             "<filter xmlns='http://indexdata.dk/yp2/config/1'\n"
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
