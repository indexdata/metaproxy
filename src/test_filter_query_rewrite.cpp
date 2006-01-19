/* $Id: test_filter_query_rewrite.cpp,v 1.1 2006-01-19 12:18:09 marc Exp $
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

class FilterBounceZ3950: public yp2::filter::Base {
public:
    void process(yp2::Package & package) const {
        
        if (package.session().is_closed())
        {
            std::cout << "Got Close.\n";
        }
       
        Z_GDU *gdu = package.request().get();
        
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_initRequest)
        {
            std::cout << "Got Z3950 Init PDU\n";         
            //Z_InitRequest *req = gdu->u.z3950->u.initRequest;
            //package.request() = gdu;
        } 
        else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_searchRequest)
        {
            std::cout << "Got Z3950 Search PDU\n";   
            //Z_SearchRequest *req = gdu->u.z3950->u.searchRequest;
            //package.request() = gdu;
        } 
        else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_scanRequest)
        {
            std::cout << "Got Z3950 Scan PDU\n";   
            //Z_ScanRequest *req = gdu->u.z3950->u.scanRequest;
            //package.request() = gdu;
        } 
        
        package.move();
    };
};

void check_query_rewrite_init(yp2::RouterChain &router)
{
   std::cout << "QUERY REWRITE INIT\n";

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

    std::cout << "Z_GDU " << z_gdu << "\n";
    BOOST_CHECK(z_gdu);
    if (z_gdu) {
        BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
        BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_initRequest);
    }
}
void check_query_rewrite_search(yp2::RouterChain &router, 
                                std::string query_in,
                                std::string query_out)
{
    std::cout << "QUERY REWRITE SEARCH " 
              << query_in << " " << query_out << "\n";
    
    // Create package with Z39.50 search request in it
    yp2::Package pack;
        
    yp2::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchRequest);

    // create package PQF query here    
    yp2::util::pqf(odr, apdu, query_in);

  
    //apdu->u.searchRequest->num_databaseNames = 1;
    //apdu->u.searchRequest->databaseNames = (char**)
    //odr_malloc(odr, sizeof(char *));
    //apdu->u.searchRequest->databaseNames[0] = odr_strdup(odr, "Default");
      

    pack.request() = apdu;
    // Done creating query. 
    
    // Put it in router
    //pack.router(router).move(); 
    
    // Inspect bounced back request
    //yazpp_1::GDU *gdu = &pack.response();
    yazpp_1::GDU *gdu = &pack.request();
    
    Z_GDU *z_gdu = gdu->get();
    //std::cout << "Z_GDU " << z_gdu << "\n";
    
    //BOOST_CHECK(z_gdu);
    if (z_gdu) {
        BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
        BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_searchRequest);
        // take query out of package again
        BOOST_CHECK_EQUAL(query_in, query_out);
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
        check_query_rewrite_search(router, "@attr 1=4 the", "@attr 1=4 the");

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
