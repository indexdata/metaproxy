/* $Id: test_filter_virt_db.cpp,v 1.14 2006-09-28 11:56:54 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
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

namespace mp = metaproxy_1;

BOOST_AUTO_UNIT_TEST( test_filter_virt_db_1 )
{
    try 
    {
        mp::filter::Virt_db vdb;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_UNIT_TEST( test_filter_virt_db_2 )
{
    try 
    {
        mp::RouterChain router;
        
        mp::filter::Virt_db vdb;
        
        router.append(vdb);
        
        // Create package with Z39.50 init request in it
        // Since there is not vhost given, the virt will make its
        // own init response (regardless of backend)
        mp::Package pack;
        
        mp::odr odr;
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


static void init(mp::Package &pack, mp::Router &router)
{
    // Create package with Z39.50 init request in it
    mp::odr odr;
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
                 
static void search(mp::Package &pack, mp::Router &router,
                   const std::string &query, const char *db,
                   const char *setname)
{
    // Create package with Z39.50 search request in it
            
    mp::odr odr;
    Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchRequest);

    mp::util::pqf(odr, apdu, query);

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

static void present(mp::Package &pack, mp::Router &router,
                    int start, int number,
                    const char *setname)
{
    // Create package with Z39.50 present request in it
            
    mp::odr odr;
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

BOOST_AUTO_UNIT_TEST( test_filter_virt_db_3 )
{
    try 
    {
        mp::RouterChain router;

        mp::filter::Log filter_log1("FRONT");
#if 0
        router.append(filter_log1);
#endif
   
        mp::filter::Virt_db vdb;        
        router.append(vdb);
        vdb.add_map_db2target("Default", "localhost:210", "");
        mp::filter::Log filter_log2("BACK");
#if 0
        router.append(filter_log2);
#endif
        mp::filter::BackendTest btest;
        router.append(btest);

        mp::Session session1;
        mp::Origin origin1;
        
        {
            mp::Package pack(session1, origin1);
            init(pack, router);
        }
        {
            // search for database for which there is no map
            mp::Package pack(session1, origin1);
            search(pack, router, "computer", "bad_database", "default");
        }
        {
            // search for database for which there a map
            mp::Package pack(session1, origin1);
            search(pack, router, "other", "Default", "default");
        }
        {
            // present from last search
            mp::Package pack(session1, origin1);
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
