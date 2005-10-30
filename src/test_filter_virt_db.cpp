/* $Id: test_filter_virt_db.cpp,v 1.8 2005-10-30 17:13:36 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "util.hpp"
#include "filter_virt_db.hpp"
#include "filter_backend_test.hpp"
#include "filter_log.hpp"

#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include <yaz/zgdu.h>
#include <yaz/pquery.h>
#include <yaz/otherinfo.h>
using namespace boost::unit_test;


BOOST_AUTO_TEST_CASE( test_filter_virt_db_1 )
{
    try 
    {
        yp2::filter::Virt_db vdb;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_virt_db_2 )
{
    try 
    {
        yp2::RouterChain router;
        
        yp2::filter::Virt_db vdb;
        
        router.append(vdb);
        
        // Create package with Z39.50 init request in it
        // Since there is not vhost given, the virt will make its
        // own init response (regardless of backend)
        yp2::Package pack;
        
        yp2::odr odr;
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_initRequest);
        
        BOOST_CHECK(apdu);
        
        pack.request() = apdu;
        
        // Put it in router
        pack.router(router).move(); 
        
        // Inspect that we got Z39.50 init Response OK.
        yazpp_1::GDU *gdu = &pack.response();
        
        BOOST_CHECK(!pack.session().is_closed()); 
        
        Z_GDU *z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_initResponse);
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}


static void init(yp2::Package &pack, yp2::Router &router)
{
    // Create package with Z39.50 init request in it
    yp2::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_initRequest);
    
    BOOST_CHECK(apdu);
    if (!apdu)
        return;
    
    pack.request() = apdu;
    
    // Put it in router
    pack.router(router).move(); 
    
    // Inspect that we got Z39.50 init response
    yazpp_1::GDU *gdu = &pack.response();
    
    BOOST_CHECK(!pack.session().is_closed()); 
    
    Z_GDU *z_gdu = gdu->get();
    BOOST_CHECK(z_gdu);
    if (!z_gdu)
        return;
    BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
    BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_initResponse);
}
                 
static void search(yp2::Package &pack, yp2::Router &router,
                   const std::string &query, const char *db,
                   const char *setname)
{
    // Create package with Z39.50 search request in it
            
    yp2::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchRequest);

    yp2::util::pqf(odr, apdu, query);

    apdu->u.searchRequest->resultSetName = odr_strdup(odr, setname);
    
    apdu->u.searchRequest->num_databaseNames = 1;
    apdu->u.searchRequest->databaseNames = (char**)
        odr_malloc(odr, sizeof(char *));
    apdu->u.searchRequest->databaseNames[0] = odr_strdup(odr, db);
    
    BOOST_CHECK(apdu);
    if (!apdu)
        return;
    
    pack.request() = apdu;
    
    Z_GDU *gdu_test = pack.request().get();
    BOOST_CHECK(gdu_test);
    
    // Put it in router
    pack.router(router).move(); 
    
    // Inspect that we got Z39.50 search response
    yazpp_1::GDU *gdu = &pack.response();
    
    BOOST_CHECK(!pack.session().is_closed()); 
    
    Z_GDU *z_gdu = gdu->get();
    BOOST_CHECK(z_gdu);
    if (!z_gdu)
        return;
    BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
    BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_searchResponse);
}

static void present(yp2::Package &pack, yp2::Router &router,
                    int start, int number,
                    const char *setname)
{
    // Create package with Z39.50 present request in it
            
    yp2::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_presentRequest);
    
    apdu->u.presentRequest->resultSetId  = odr_strdup(odr, setname);
    *apdu->u.presentRequest->resultSetStartPoint = start;
    *apdu->u.presentRequest->numberOfRecordsRequested = number;

    BOOST_CHECK(apdu);
    if (!apdu)
        return;
    
    pack.request() = apdu;
    
    Z_GDU *gdu_test = pack.request().get();
    BOOST_CHECK(gdu_test);
    
    // Put it in router
    pack.router(router).move(); 
    
    // Inspect that we got Z39.50 present response
    yazpp_1::GDU *gdu = &pack.response();
    
    BOOST_CHECK(!pack.session().is_closed()); 
    
    Z_GDU *z_gdu = gdu->get();
    BOOST_CHECK(z_gdu);
    if (!z_gdu)
        return;
    BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
    BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_presentResponse);
}

BOOST_AUTO_TEST_CASE( test_filter_virt_db_3 )
{
    try 
    {
        yp2::RouterChain router;

        yp2::filter::Log filter_log1("FRONT");
#if 0
        router.append(filter_log1);
#endif
   
        yp2::filter::Virt_db vdb;        
        router.append(vdb);
        vdb.add_map_db2vhost("Default", "localhost:210");
        yp2::filter::Log filter_log2("BACK");
#if 0
        router.append(filter_log2);
#endif
        yp2::filter::Backend_test btest;
        router.append(btest);

        yp2::Session session1;
        yp2::Origin origin1;
        
        {
            yp2::Package pack(session1, origin1);
            init(pack, router);
        }
        {
            // search for database for which there is no map
            yp2::Package pack(session1, origin1);
            search(pack, router, "computer", "bad_database", "default");
        }
        {
            // search for database for which there a map
            yp2::Package pack(session1, origin1);
            search(pack, router, "other", "Default", "default");
        }
        {
            // present from last search
            yp2::Package pack(session1, origin1);
            present(pack, router, 1, 2, "default");
        }
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
