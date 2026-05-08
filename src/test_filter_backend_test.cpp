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
#include <stdexcept>

#include <metaproxy/util.hpp>
#include "filter_backend_test.hpp"
#include "filter_log.hpp"

#include <metaproxy/router_chain.hpp>
#include <metaproxy/package.hpp>

#include <yaz/zgdu.h>
#include <yaz/pquery.h>
#include <yaz/otherinfo.h>
#include <yaz/oid_std.h>
#include <yaz/diagbib1.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;

namespace mp = metaproxy_1;

BOOST_AUTO_TEST_CASE( test_filter_backend_test_construct )
{
    try
    {
        mp::filter::BackendTest btest;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_backend_test_search_present )
{
    try
    {
        mp::RouterChain router;

        mp::filter::BackendTest btest;
        router.append(btest);

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
        apdu = zget_APDU(odr, Z_APDU_searchRequest);

        mp::util::pqf(odr, apdu, "computer");

        apdu->u.searchRequest->num_databaseNames = 1;
        apdu->u.searchRequest->databaseNames = (char**)
            odr_malloc(odr, sizeof(char *));
        apdu->u.searchRequest->databaseNames[0] = odr_strdup(odr, "Default");

        BOOST_CHECK(apdu);

        pack.request() = apdu;

        // Put it in router
        pack.router(router).move();

        // Inspect that we got response
        gdu = &pack.response();

        BOOST_CHECK(!pack.session().is_closed());

        z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_searchResponse);
        }

        // present request no syntax, expecing usmarc record
        apdu = zget_APDU(odr, Z_APDU_presentRequest);
        BOOST_CHECK(apdu);
        apdu->u.presentRequest->resultSetStartPoint = odr_intdup(odr, 1);
        apdu->u.presentRequest->numberOfRecordsRequested = odr_intdup(odr, 1);

        pack.request() = apdu;
        pack.router(router).move();
        gdu = &pack.response();
        BOOST_CHECK(!pack.session().is_closed());

        z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_presentResponse);
            Z_PresentResponse *resp = z_gdu->u.z3950->u.presentResponse;
            BOOST_CHECK(resp->records);
            BOOST_CHECK(*resp->numberOfRecordsReturned == 1);
            BOOST_CHECK(*resp->nextResultSetPosition == 2);
            BOOST_CHECK(resp->records->which == Z_Records_DBOSD);
            BOOST_CHECK(resp->records->u.databaseOrSurDiagnostics->num_records == 1);
            BOOST_CHECK(resp->records->u.databaseOrSurDiagnostics->records[0]->which == Z_NamePlusRecord_databaseRecord);
            Z_NamePlusRecord *npr = resp->records->u.databaseOrSurDiagnostics->records[0];
            BOOST_CHECK(npr->u.databaseRecord);
            BOOST_CHECK_EQUAL(npr->u.databaseRecord->which, Z_External_octet);
            BOOST_CHECK_EQUAL(oid_oidcmp(npr->u.databaseRecord->direct_reference, yaz_oid_recsyn_usmarc), 0);
        }

        // opac syntax, expecing usmarc record
        apdu = zget_APDU(odr, Z_APDU_presentRequest);
        BOOST_CHECK(apdu);
        apdu->u.presentRequest->resultSetStartPoint = odr_intdup(odr, 1);
        apdu->u.presentRequest->numberOfRecordsRequested = odr_intdup(odr, 1);
        apdu->u.presentRequest->preferredRecordSyntax = odr_oiddup(odr, yaz_oid_recsyn_opac);

        pack.request() = apdu;
        pack.router(router).move();
        gdu = &pack.response();
        BOOST_CHECK(!pack.session().is_closed());

        z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_presentResponse);
            Z_PresentResponse *resp = z_gdu->u.z3950->u.presentResponse;
            BOOST_CHECK(resp->records);
            BOOST_CHECK_EQUAL(*resp->numberOfRecordsReturned, 1);
            BOOST_CHECK_EQUAL(*resp->nextResultSetPosition, 2);
            BOOST_CHECK_EQUAL(resp->records->which, Z_Records_DBOSD);
            BOOST_CHECK_EQUAL(resp->records->u.databaseOrSurDiagnostics->num_records, 1);
            BOOST_CHECK_EQUAL(resp->records->u.databaseOrSurDiagnostics->records[0]->which, Z_NamePlusRecord_databaseRecord);
            Z_NamePlusRecord *npr = resp->records->u.databaseOrSurDiagnostics->records[0];
            BOOST_CHECK(npr->u.databaseRecord);
            BOOST_CHECK_EQUAL(npr->u.databaseRecord->which, Z_External_OPAC);
            BOOST_CHECK_EQUAL(oid_oidcmp(npr->u.databaseRecord->direct_reference, yaz_oid_recsyn_opac), 0);
        }

        // danmarc syntax, expecing non surrogate diagnostic
        apdu = zget_APDU(odr, Z_APDU_presentRequest);
        BOOST_CHECK(apdu);
        apdu->u.presentRequest->resultSetStartPoint = odr_intdup(odr, 1);
        apdu->u.presentRequest->numberOfRecordsRequested = odr_intdup(odr, 1);
        apdu->u.presentRequest->preferredRecordSyntax = odr_oiddup(odr, yaz_oid_recsyn_danmarc);

        pack.request() = apdu;
        pack.router(router).move();
        gdu = &pack.response();
        BOOST_CHECK(!pack.session().is_closed());

        z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_presentResponse);
            Z_PresentResponse *resp = z_gdu->u.z3950->u.presentResponse;
            BOOST_CHECK(resp->records);
            BOOST_CHECK_EQUAL(*resp->numberOfRecordsReturned, 0);
            BOOST_CHECK_EQUAL(resp->records->which, Z_Records_NSD);
            Z_DefaultDiagFormat *diag = resp->records->u.nonSurrogateDiagnostic;
            BOOST_CHECK(diag);
            BOOST_CHECK_EQUAL(diag->which, Z_DefaultDiagFormat_v2Addinfo);
            BOOST_CHECK_EQUAL(*diag->condition, YAZ_BIB1_RECORD_SYNTAX_UNSUPP);
        }

        // present request to get surrogate diagnostic
        apdu = zget_APDU(odr, Z_APDU_presentRequest);
        BOOST_CHECK(apdu);
        apdu->u.presentRequest->resultSetStartPoint = odr_intdup(odr, 1);
        apdu->u.presentRequest->numberOfRecordsRequested = odr_intdup(odr, 1);
        apdu->u.presentRequest->recordComposition = (Z_RecordComposition *) odr_malloc(odr, sizeof(Z_RecordComposition));
        apdu->u.presentRequest->recordComposition->which = Z_RecordComp_simple;
        apdu->u.presentRequest->recordComposition->u.simple = (Z_ElementSetNames *) odr_malloc(odr, sizeof(Z_ElementSetNames));
        apdu->u.presentRequest->recordComposition->u.simple->which = Z_ElementSetNames_generic;
        apdu->u.presentRequest->recordComposition->u.simple->u.generic = odr_strdup(odr, "SD");

        pack.request() = apdu;
        pack.router(router).move();
        gdu = &pack.response();
        BOOST_CHECK(!pack.session().is_closed());

        z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_presentResponse);
            Z_PresentResponse *resp = z_gdu->u.z3950->u.presentResponse;
            BOOST_CHECK(resp->records);
            BOOST_CHECK_EQUAL(*resp->numberOfRecordsReturned, 1);
            BOOST_CHECK_EQUAL(*resp->nextResultSetPosition, 2);
            BOOST_CHECK_EQUAL(resp->records->which, Z_Records_DBOSD);
            BOOST_CHECK_EQUAL(resp->records->u.databaseOrSurDiagnostics->num_records, 1);
            BOOST_CHECK_EQUAL(resp->records->u.databaseOrSurDiagnostics->records[0]->which, Z_NamePlusRecord_surrogateDiagnostic);
            Z_DiagRec *diagRec = resp->records->u.databaseOrSurDiagnostics->records[0]->u.surrogateDiagnostic;
            BOOST_CHECK(diagRec);
            BOOST_CHECK_EQUAL(diagRec->which, Z_DiagRec_defaultFormat);
            BOOST_CHECK_EQUAL(*diagRec->u.defaultFormat->condition, YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_);
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_backend_test_no_init_search )
{
    try
    {
        mp::RouterChain router;

        mp::filter::BackendTest btest;
        router.append(btest);

        mp::Package pack;

        // send search request as first request.. That should fail with
        // a close from the backend
        mp::odr odr;
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchRequest);

        mp::util::pqf(odr, apdu, "computer");

        apdu->u.searchRequest->num_databaseNames = 1;
        apdu->u.searchRequest->databaseNames = (char**)
            odr_malloc(odr, sizeof(char *));
        apdu->u.searchRequest->databaseNames[0] = odr_strdup(odr, "Default");

        BOOST_CHECK(apdu);

        pack.request() = apdu;

        // Put it in router
        pack.router(router).move();

        // Inspect that we got Z39.50 close
        yazpp_1::GDU *gdu = &pack.response();

        BOOST_CHECK(pack.session().is_closed());

        Z_GDU *z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_close);
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_backend_test_no_init_present )
{
    try
    {
        mp::RouterChain router;

        mp::filter::BackendTest btest;
        router.append(btest);

        mp::Package pack;

        // send present request as first request.. That should fail with
        // a close from the backend
        mp::odr odr;
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_presentRequest);

        BOOST_CHECK(apdu);

        pack.request() = apdu;

        // Put it in router
        pack.router(router).move();

        // Inspect that we got Z39.50 close
        yazpp_1::GDU *gdu = &pack.response();

        BOOST_CHECK(pack.session().is_closed());

        Z_GDU *z_gdu = gdu->get();
        BOOST_CHECK(z_gdu);
        if (z_gdu) {
            BOOST_CHECK_EQUAL(z_gdu->which, Z_GDU_Z3950);
            BOOST_CHECK_EQUAL(z_gdu->u.z3950->which, Z_APDU_close);
        }
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

