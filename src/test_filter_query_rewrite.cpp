/* $Id: test_filter_query_rewrite.cpp,v 1.4 2006-01-23 08:12:24 mike Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "filter_query_rewrite.hpp"
#include "util.hpp"
#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
using namespace yp2::util;

class FilterBounceZ3950: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {
        
        if (package.session().is_closed())
        {
            std::cout << "Got Close.\n";
            return;
        }
       
        Z_GDU *gdu = package.request().get();
        
        if (gdu && gdu->which == Z_GDU_Z3950 
            && gdu->u.z3950->which == Z_APDU_initRequest)
        {
            std::cout << "Got Z3950 Init PDU\n";         
            //Z_InitRequest *req = gdu->u.z3950->u.initRequest;
            //package.request() = gdu;
            return;
        } 
        else if (gdu && gdu->which == Z_GDU_Z3950 
                 && gdu->u.z3950->which == Z_APDU_searchRequest)
        {
            std::cout << "Got Z3950 Search PDU\n";   
            //Z_SearchRequest *req = gdu->u.z3950->u.searchRequest;
            //package.request() = gdu;
            return;
        } 
        else if (gdu && gdu->which == Z_GDU_Z3950 
                 && gdu->u.z3950->which == Z_APDU_scanRequest)
        {
            std::cout << "Got Z3950 Scan PDU\n";   
            //Z_ScanRequest *req = gdu->u.z3950->u.scanRequest;
            //package.request() = gdu;
            return;
        } 
        
        package.move();
    };
};

void check_query_rewrite_init(yp2::RouterChain &router)
{
    //std::cout << "QUERY REWRITE INIT\n";

    // Create package with Z39.50 init request in it
    yp2::Package pack;
        
    yp2::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_initRequest);
    
    pack.request() = apdu;
    // Done creating query.

    // Put it in router
    pack.router(router).move(); 
    
    // Inspect bounced back request
    //yazpp_1::GDU *gdu = &pack.response();
    yazpp_1::GDU *gdu = &pack.request();
    
    Z_GDU *z_gdu = gdu->get();

    //std::cout << "Z_GDU " << z_gdu << "\n";
    BOOST_CHECK(z_gdu);
    if (z_gdu) {
        BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
        BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_initRequest);
    }
}

void check_query_rewrite_search(yp2::RouterChain &router, 
                                std::string query_in,
                                std::string query_expect)
{
    //std::cout << "QUERY REWRITE SEARCH " 
    //          << query_in << " " << query_expect << "\n";
    
    // Create package with Z39.50 search request in it
    yp2::Package pack;
        
    yp2::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchRequest);

    // create package PQF query here    
    yp2::util::pqf(odr, apdu, query_in);

    // create package PDF database info (needed!)
    apdu->u.searchRequest->num_databaseNames = 1;
    apdu->u.searchRequest->databaseNames 
        = (char**)odr_malloc(odr, sizeof(char *));
    apdu->u.searchRequest->databaseNames[0] = odr_strdup(odr, "Default");

    // Done creating request package  
    pack.request() = apdu;
    
    // Put it in router
    pack.router(router).move(); 
    
    // Inspect bounced back request
    //yazpp_1::GDU *gdu = &pack.response();
    yazpp_1::GDU *gdu = &pack.request();
    
    Z_GDU *z_gdu = gdu->get();
    //std::cout << "Z_GDU " << z_gdu << "\n";
    
    BOOST_CHECK(z_gdu);
    if (z_gdu) {
        BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
        BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_searchRequest);

        // take query out of package again and check rewrite
        std::string query_changed 
            = zQueryToString(z_gdu->u.z3950->u.searchRequest->query);
        BOOST_CHECK_EQUAL(query_expect, query_changed);
    }
}


BOOST_AUTO_UNIT_TEST( test_filter_query_rewrite_1 )
{
    try 
    {
        yp2::filter::QueryRewrite f_query_rewrite;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_UNIT_TEST( test_filter_query_rewrite2 )
{
    try 
    {
        yp2::RouterChain router;
        
        yp2::filter::QueryRewrite f_query_rewrite;
        //FilterBounceZ3950 f_bounce_z3950;
        
        router.append(f_query_rewrite);
        //router.append(f_bounce_z3950);

        check_query_rewrite_init(router);
        check_query_rewrite_search(router, 
                                   "@attrset Bib-1 @attr 1=4 the", 
                                   "@attrset Bib-1 @attr 1=4 the");

    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}


BOOST_AUTO_UNIT_TEST( test_filter_query_rewrite3 )
{
    

    try 
    {
        yp2::RouterChain router;
        

        std::string xmlconf = 
            "<?xml version='1.0'?>\n"
            "<filter xmlns='http://indexdata.dk/yp2/config/1'\n"
            "        id='qrw1' type='query_rewrite'>\n"
            "  <regex action='all'>\n"
            "    <expression>@attrset XYZ</expression>\n"
            "    <format>@attrset Bib-1</format>\n"
            "  </regex>\n"
            "  <regex action='search'>\n"
            "    <expression>@attr 1=4</expression>\n"
            "    <format>@attr 1=4 @attr 4=2</format>\n"
            "  </regex>\n"
            "  <regex action='search'>\n"
            "    <expression>fish</expression>\n"
            "    <format>cat</format>\n"
            "  </regex>\n"
            "  <regex action='scan'>\n"
            "    <expression>@attr 1=4</expression>\n"
            "    <format>@attr 1=5 @attr 4=1</format>\n"
            "  </regex>\n"
            "  <regex action='scan'>\n"
            "    <expression>fish</expression>\n"
            "    <format>mouse</format>\n"
            "  </regex>\n"
            "  <regex action='finally'>\n"
            "    <expression>^</expression>\n"
            "    <format>@and @attr1=9999 vdb</format>\n"
            "  </regex>\n"
            "</filter>\n"
            ;
         
        //std::cout << xmlconf  << std::endl;

        // reading and parsing XML conf
        xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(), xmlconf.size());
        BOOST_CHECK(doc);
        xmlNode *root_element = xmlDocGetRootElement(doc);

        // creating and configuring filter
        yp2::filter::QueryRewrite f_query_rewrite;
        f_query_rewrite.configure(root_element);
        
        // remeber to free XML DOM
        xmlFreeDoc(doc);
        
        // add only filter to router
        router.append(f_query_rewrite);

        // start testing
        check_query_rewrite_init(router);
        check_query_rewrite_search(router, 
                                   "@attrset Bib-1 @attr 1=4 the", 
                                   "@attrset Bib-1 @attr 1=4 the");

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
