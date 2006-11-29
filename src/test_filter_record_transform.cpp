/* $Id: test_filter_record_transform.cpp,v 1.3 2006-11-29 13:00:54 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter_record_transform.hpp"
//#include "util.hpp"
//#include "sru_util.hpp"
#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

//#include <iostream>
//#include <stdexcept>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include <iostream>


using namespace boost::unit_test;

namespace mp = metaproxy_1;
//using namespace mp::util;



BOOST_AUTO_UNIT_TEST( test_filter_record_transform_1 )
{
    try 
    {
        mp::filter::RecordTransform f_rec_trans;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_UNIT_TEST( test_filter_record_transform_2 )
{
    try 
    {
        mp::RouterChain router;
        
        mp::filter::RecordTransform f_rec_trans;
        
        router.append(f_rec_trans);

        //check_sru_to_z3950_init(router);
        //check_sru_to_z3950_search(router, 
        //                           "@attrset Bib-1 @attr 1=4 the", 
        //                           "@attrset Bib-1 @attr 1=4 the");

    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}


BOOST_AUTO_UNIT_TEST( test_filter_record_transform_3 )
{
    

    try 
    {
        mp::RouterChain router;
        

        std::string xmlconf = 
            "<?xml version='1.0'?>\n"
            "<filter xmlns='http://indexdata.com/metaproxy'\n"
            "        id='rec_trans_1' type='record_transform'>\n"
            "<retrievalinfo>"
            "<retrieval" 
            " syntax=\"usmarc\""
            " name=\"marcxml\""
            " backendsyntax=\"usmarc\""
            " backendname=\"marcxml\""
            " identifier=\"info:srw/schema/1/marcxml-v1.1\""
            ">"
            "<convert/>"
            "</retrieval>"
            "</retrievalinfo>"
            "</filter>\n"
            ;
         
        //std::cout << xmlconf  << std::endl;

        // reading and parsing XML conf
        xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(), xmlconf.size());
        BOOST_CHECK(doc);
        xmlNode *root_element = xmlDocGetRootElement(doc);

        // creating and configuring filter
        mp::filter::RecordTransform f_rec_trans;
        f_rec_trans.configure(root_element);
        
        // remeber to free XML DOM
        xmlFreeDoc(doc);
        
        // add only filter to router
        router.append(f_rec_trans);

        // start testing
        //check_sru_to_z3950_init(router);
        //check_sru_to_z3950_search(router, 
        //                           "@attrset Bib-1 @attr 1=4 the", 
        //                           "@attrset Bib-1 @attr 1=4 the");

    }

    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }

    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
