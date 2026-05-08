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

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include "filter_backend_test.hpp"

#include <stdexcept>
#include <list>
#include <map>
#include <iostream>

#include <boost/thread/mutex.hpp>

#include <yaz/zgdu.h>
#include <yaz/log.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>
#include <yaz/oid_db.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;
using namespace mp;

namespace metaproxy_1 {
    namespace filter {
        class Session_info {
            int dummy;
        public:
            Session_info() { dummy = 0; };
        };
        class BackendTest::Rep {
            friend class BackendTest;

            Z_Records *fetch(
                ODR odr, Odr_oid *preferredRecordSyntax,
                Z_ElementSetNames *esn,
                int start, int number, int &error_code, std::string &addinfo,
                int *number_returned, int *next_position);

            bool m_support_named_result_sets;

            session_map<Session_info> m_sessions;
        };
    }
}


static const int result_set_size = 42;

// an ISO2709 USMARC/MARC21 record that we return..
static const char *marc_record =
  "\x30\x30\x33\x36\x36\x6E\x61\x6D\x20\x20\x32\x32\x30\x30\x31\x36"
  "\x39\x38\x61\x20\x34\x35\x30\x30\x30\x30\x31\x30\x30\x31\x33\x30"
  "\x30\x30\x30\x30\x30\x30\x33\x30\x30\x30\x34\x30\x30\x30\x31\x33"
  "\x30\x30\x35\x30\x30\x31\x37\x30\x30\x30\x31\x37\x30\x30\x38\x30"
  "\x30\x34\x31\x30\x30\x30\x33\x34\x30\x31\x30\x30\x30\x31\x37\x30"
  "\x30\x31\x37\x39\x30\x34\x30\x30\x30\x31\x33\x30\x30\x30\x37\x35"
  "\x30\x35\x30\x30\x30\x31\x32\x30\x30\x30\x38\x38\x31\x30\x30\x30"
  "\x30\x31\x37\x30\x30\x31\x30\x30\x32\x34\x35\x30\x30\x33\x30\x30"
  "\x30\x31\x31\x37\x32\x36\x30\x30\x30\x31\x32\x30\x30\x31\x34\x37"
  "\x32\x36\x33\x30\x30\x30\x39\x30\x30\x31\x35\x39\x33\x30\x30\x30"
  "\x30\x31\x31\x30\x30\x31\x36\x38\x1E\x20\x20\x20\x31\x31\x32\x32"
  "\x34\x34\x36\x36\x20\x1E\x44\x4C\x43\x1E\x30\x30\x30\x30\x30\x30"
  "\x30\x30\x30\x30\x30\x30\x30\x30\x2E\x30\x1E\x39\x31\x30\x37\x31"
  "\x30\x63\x31\x39\x39\x31\x30\x37\x30\x31\x6E\x6A\x75\x20\x20\x20"
  "\x20\x20\x20\x20\x20\x20\x20\x20\x30\x30\x30\x31\x30\x20\x65\x6E"
  "\x67\x20\x20\x1E\x20\x20\x1F\x61\x44\x4C\x43\x1F\x63\x44\x4C\x43"
  "\x1E\x30\x30\x1F\x61\x31\x32\x33\x2D\x78\x79\x7A\x1E\x31\x30\x1F"
  "\x61\x4A\x61\x63\x6B\x20\x43\x6F\x6C\x6C\x69\x6E\x73\x1E\x31\x30"
  "\x1F\x61\x48\x6F\x77\x20\x74\x6F\x20\x70\x72\x6F\x67\x72\x61\x6D"
  "\x20\x61\x20\x63\x6F\x6D\x70\x75\x74\x65\x72\x1E\x31\x20\x1F\x61"
  "\x50\x65\x6E\x67\x75\x69\x6E\x1E\x20\x20\x1F\x61\x38\x37\x31\x30"
  "\x1E\x20\x20\x1F\x61\x70\x2E\x20\x63\x6D\x2E\x1E\x20\x20\x1F\x61"
  "\x20\x20\x20\x31\x31\x32\x32\x34\x34\x36\x36\x20\x1E\x1D";


yf::BackendTest::BackendTest() : m_p(new BackendTest::Rep) {
    m_p->m_support_named_result_sets = false;
}

yf::BackendTest::~BackendTest() {
}


Z_OPACRecord *dummy_opac(const char *item_id, const char *element_set_name, ODR odr, const char *marc_input)
{
    Z_OPACRecord *rec;
    rec = (Z_OPACRecord *) odr_malloc(odr, sizeof(*rec));
    rec->bibliographicRecord =
        z_ext_record_usmarc(odr, marc_input, strlen(marc_input));
    int availableNow = strcmp(element_set_name, "F") == 0;
    rec->num_holdingsData = 1;
    rec->holdingsData = (Z_HoldingsRecord **)
        odr_malloc(odr, sizeof(*rec->holdingsData));
    for (int i = 0; i < rec->num_holdingsData; i++)
    {
        Z_HoldingsRecord *hr = (Z_HoldingsRecord *)
            odr_malloc(odr, sizeof(*hr));
        Z_HoldingsAndCircData *hc = (Z_HoldingsAndCircData *)
            odr_malloc(odr, sizeof(*hc));

        rec->holdingsData[i] = hr;
        hr->which = Z_HoldingsRecord_holdingsAndCirc;
        hr->u.holdingsAndCirc = hc;

        hc->typeOfRecord = (char *)"u";

        hc->encodingLevel = (char *)"u";

        hc->format = 0; /* OPT */
        hc->receiptAcqStatus = (char *)"0";
        hc->generalRetention = 0; /* OPT */
        hc->completeness = 0; /* OPT */
        hc->dateOfReport = (char *)"000000";
        hc->nucCode = (char *)"s-FM/GC";
        hc->localLocation =
            (char *)"Main or Science/Business Reading Rms - STORED OFFSITE";
        hc->shelvingLocation = 0; /* OPT */
        hc->callNumber = (char *)"MLCM 89/00602 (N)";
        hc->shelvingData = (char *)"FT MEADE";
        hc->copyNumber = (char *)"Copy 1";
        hc->publicNote = 0; /* OPT */
        hc->reproductionNote = 0; /* OPT */
        hc->termsUseRepro = 0; /* OPT */
        hc->enumAndChron = 0; /* OPT */

        hc->num_volumes = 0;
        hc->volumes = 0;

        hc->num_circulationData = 1;
        hc->circulationData = (Z_CircRecord **)
             odr_malloc(odr, sizeof(*hc->circulationData));
        hc->circulationData[0] = (Z_CircRecord *)
             odr_malloc(odr, sizeof(**hc->circulationData));

        hc->circulationData[0]->availableNow = odr_booldup(odr, availableNow);
        hc->circulationData[0]->availablityDate = 0;
        hc->circulationData[0]->availableThru = 0;
        hc->circulationData[0]->restrictions = 0;
        hc->circulationData[0]->itemId = odr_strdup(odr, item_id);
        hc->circulationData[0]->renewable = odr_booldup(odr, availableNow);
        hc->circulationData[0]->onHold = odr_booldup(odr, !availableNow);
        hc->circulationData[0]->enumAndChron = 0;
        hc->circulationData[0]->midspine = 0;
        hc->circulationData[0]->temporaryLocation = 0;
    }
    return rec;
}

Z_Records *yf::BackendTest::Rep::fetch(
    ODR odr, Odr_oid *preferredRecordSyntax,
    Z_ElementSetNames *esn,
    int start, int number, int &error_code, std::string &addinfo,
    int *number_returned, int *next_position)
{
    if (number + start - 1 > result_set_size || start < 1)
    {
        error_code = YAZ_BIB1_PRESENT_REQUEST_OUT_OF_RANGE;
        return 0;
    }

    const char *element_set_name = "F"; // default to use
    if (esn)
    {
        if (esn->which != Z_ElementSetNames_generic)
        {
            error_code
                = YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
            return 0;
        }
        element_set_name = esn->u.generic;
    }

    if (!preferredRecordSyntax)
        preferredRecordSyntax = odr_oiddup(odr, yaz_oid_recsyn_usmarc);

    if (!oid_oidcmp(preferredRecordSyntax, yaz_oid_recsyn_xml))
    {
        if (strncmp(element_set_name, "FF", 2) && strcmp(element_set_name, "SD"))
        {
            error_code
                = YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
            addinfo = std::string(element_set_name);
            return 0;
        }
    }
    else if (!oid_oidcmp(preferredRecordSyntax, yaz_oid_recsyn_usmarc) || !oid_oidcmp(preferredRecordSyntax, yaz_oid_recsyn_opac))
    {
        if (strcmp(element_set_name, "B") && strcmp(element_set_name, "F") && strcmp(element_set_name, "SD"))
        {
            error_code
                = YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_;
            addinfo = std::string(element_set_name);
            return 0;
        }
    }
    else
    {
        error_code = YAZ_BIB1_RECORD_SYNTAX_UNSUPP;
        return 0;
    }
    Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
    rec->which = Z_Records_DBOSD;
    rec->u.databaseOrSurDiagnostics = (Z_NamePlusRecordList *)
        odr_malloc(odr, sizeof(Z_NamePlusRecordList));
    rec->u.databaseOrSurDiagnostics->num_records = number;
    rec->u.databaseOrSurDiagnostics->records = (Z_NamePlusRecord **)
        odr_malloc(odr, sizeof(Z_NamePlusRecord *) * number);
    for (int i = 0; i < number; i++)
    {
        if (!strcmp(element_set_name, "SD")) {
            rec->u.databaseOrSurDiagnostics->records[i] =
                zget_surrogateDiagRec(odr, 0, YAZ_BIB1_SPECIFIED_ELEMENT_SET_NAME_NOT_VALID_FOR_SPECIFIED_,  "SD");
            continue;
        }
        rec->u.databaseOrSurDiagnostics->records[i] = (Z_NamePlusRecord *)
            odr_malloc(odr, sizeof(Z_NamePlusRecord));
        Z_NamePlusRecord *npr = rec->u.databaseOrSurDiagnostics->records[i];
        npr->databaseName = 0;
        npr->which = Z_NamePlusRecord_databaseRecord;

        if (!strncmp(element_set_name, "FF", 2))
        {   // XML test record with optional size in kilo-bytes .. eg FF10 for 10KB
            size_t sz = 1024;
            if (element_set_name[2])
                sz = atoi(element_set_name+2) * 1024;
            if (sz < 10 || sz > 1024*1024)
                sz = 10;
            char *tmp_rec = (char*) xmalloc(sz);

            memset(tmp_rec, 'a', sz);
            memcpy(tmp_rec, "<a>", 3);
            memcpy(tmp_rec + sz - 4, "</a>", 4);

            npr->u.databaseRecord = z_ext_record_xml(odr, tmp_rec, sz);
            xfree(tmp_rec);
        }
        else
        {
            char *tmp_rec = odr_strdup(odr, marc_record);
            char offset_str[30];
            snprintf(offset_str, sizeof offset_str, "test__%09d_", i+start);
            memcpy(tmp_rec+186, offset_str, strlen(offset_str));
            if (!oid_oidcmp(preferredRecordSyntax, yaz_oid_recsyn_opac)) {
                Z_OPACRecord *rec = dummy_opac(offset_str, element_set_name, odr, tmp_rec);
                npr->u.databaseRecord = z_ext_record_oid(odr, yaz_oid_recsyn_opac, (char*) rec, -1);
            }
            else
                npr->u.databaseRecord = z_ext_record_usmarc(odr, tmp_rec, strlen(tmp_rec));
        }

    }
    *number_returned = number;
    if (start + number > result_set_size)
        *next_position = 0;
    else
        *next_position = start + number;
    return rec;
}

void yf::BackendTest::process(Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950)
        package.move();
    else
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        Z_APDU *apdu_res = 0;
        mp::odr odr;

        if (apdu_req->which != Z_APDU_initRequest &&
            !m_p->m_sessions.exist(package.session()))
        {
            apdu_res = odr.create_close(apdu_req,
                                        Z_Close_protocolError,
                                        "no init for filter_backend_test");
            package.session().close();
        }
        else if (apdu_req->which == Z_APDU_initRequest)
        {
            apdu_res = odr.create_initResponse(apdu_req, 0, 0);
            Z_InitRequest *req = apdu_req->u.initRequest;
            Z_InitResponse *resp = apdu_res->u.initResponse;

            resp->implementationName = odr_strdup(odr, "backend_test");
            if (ODR_MASK_GET(req->options, Z_Options_namedResultSets))
                m_p->m_support_named_result_sets = true;

            int i;
            static const int masks[] = {
                Z_Options_search, Z_Options_present,
                Z_Options_namedResultSets, -1
            };
            for (i = 0; masks[i] != -1; i++)
                if (ODR_MASK_GET(req->options, masks[i]))
                    ODR_MASK_SET(resp->options, masks[i]);
            static const int versions[] = {
                Z_ProtocolVersion_1,
                Z_ProtocolVersion_2,
                Z_ProtocolVersion_3,
                -1
            };
            for (i = 0; versions[i] != -1; i++)
                if (ODR_MASK_GET(req->protocolVersion, versions[i]))
                    ODR_MASK_SET(resp->protocolVersion, versions[i]);
                else
                    break;

            *resp->preferredMessageSize = *req->preferredMessageSize;
            *resp->maximumRecordSize = *req->maximumRecordSize;

            Session_info info;
            m_p->m_sessions.create(info, package.session());
        }
        else if (apdu_req->which == Z_APDU_searchRequest)
        {
            Z_SearchRequest *req = apdu_req->u.searchRequest;

            if (!m_p->m_support_named_result_sets &&
                strcmp(req->resultSetName, "default"))
            {
                apdu_res =
                    odr.create_searchResponse(
                        apdu_req,  YAZ_BIB1_RESULT_SET_NAMING_UNSUPP, 0);
            }
            else
            {
                Z_Records *records = 0;
                int number_returned = 0;
                int next_position = 0;
                int error_code = 0;
                std::string addinfo;
                const char *element_set_name = 0;

                Odr_int number = 0;
                mp::util::piggyback_sr(req, result_set_size,
                                       number, &element_set_name);

                if (number)
                {   // not a large set for sure
                    Z_ElementSetNames *esn;
                    if (number > *req->smallSetUpperBound)
                        esn = req->mediumSetElementSetNames;
                    else
                        esn = req->smallSetElementSetNames;
                    records = m_p->fetch(
                        odr, req->preferredRecordSyntax, esn,
                        1, number,
                        error_code, addinfo,
                        &number_returned,
                        &next_position);
                }
                if (error_code)
                {
                    apdu_res =
                        odr.create_searchResponse(
                            apdu_req, error_code, addinfo.c_str());
                    Z_SearchResponse *resp = apdu_res->u.searchResponse;
                    *resp->resultCount = result_set_size;
                }
                else
                {
                    apdu_res =
                        odr.create_searchResponse(apdu_req, 0, 0);
                    Z_SearchResponse *resp = apdu_res->u.searchResponse;
                    *resp->resultCount = result_set_size;
                    *resp->numberOfRecordsReturned = number_returned;
                    *resp->nextResultSetPosition = next_position;
                    resp->records = records;
                }
            }
        }
        else if (apdu_req->which == Z_APDU_presentRequest)
        {
            Z_PresentRequest *req = apdu_req->u.presentRequest;
            int number_returned = 0;
            int next_position = 0;
            int error_code = 0;
            std::string addinfo;
            Z_ElementSetNames *esn = 0;

            if (req->recordComposition)
            {
                if (req->recordComposition->which == Z_RecordComp_simple)
                    esn = req->recordComposition->u.simple;
                else
                {
                    apdu_res =
                        odr.create_presentResponse(
                            apdu_req,
                            YAZ_BIB1_ONLY_A_SINGLE_ELEMENT_SET_NAME_SUPPORTED,
                            0);
                    package.response() = apdu_res;
                    return;
                }
            }
            Z_Records *records = m_p->fetch(
                odr, req->preferredRecordSyntax, esn,
                *req->resultSetStartPoint, *req->numberOfRecordsRequested,
                error_code, addinfo,
                &number_returned,
                &next_position);

            if (error_code)
            {
                apdu_res =
                    odr.create_presentResponse(apdu_req, error_code,
                                               addinfo.c_str());
            }
            else
            {
                apdu_res =
                    odr.create_presentResponse(apdu_req, 0, 0);
                Z_PresentResponse *resp = apdu_res->u.presentResponse;
                resp->records = records;
                *resp->numberOfRecordsReturned = number_returned;
                *resp->nextResultSetPosition = next_position;
            }
        }
        else if (apdu_req->which == Z_APDU_close)
        {
            apdu_res = odr.create_close(apdu_req,
                                        Z_Close_finished, 0);
            package.session().close();
        }
        else
        {
            apdu_res = odr.create_close(apdu_req,
                                        Z_Close_protocolError,
                                        "backend_test: unhandled APDU");
            package.session().close();
        }
        if (apdu_res)
            package.response() = apdu_res;
    }
    if (package.session().is_closed())
        m_p->m_sessions.release(package.session());
}

void mp::filter::BackendTest::configure(const xmlNode * ptr, bool test_only,
                                        const char *path)
{
    mp::xml::check_empty(ptr);
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::BackendTest;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_backend_test = {
        0,
        "backend_test",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
