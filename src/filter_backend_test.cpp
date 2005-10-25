/* $Id: filter_backend_test.cpp,v 1.2 2005-10-25 15:19:39 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "filter_backend_test.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <list>
#include <map>
#include <iostream>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class Backend_test::Rep {
            friend class Backend_test;
            
        private:
            bool m_support_named_result_sets;
        };
    }
}

yf::Backend_test::Backend_test() {
    m_p = new Backend_test::Rep;
    m_p->m_support_named_result_sets = false;
}

yf::Backend_test::~Backend_test() {
    delete m_p;
}

void yf::Backend_test::process(Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950)
        package.move();
    else
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        Z_APDU *apdu_res = 0;
        ODR odr = odr_createmem(ODR_ENCODE);
        if (apdu_req->which == Z_APDU_initRequest)
        {
            apdu_res = zget_APDU(odr, Z_APDU_initResponse);
            Z_InitRequest *req = apdu_req->u.initRequest;
            Z_InitResponse *resp = apdu_res->u.initResponse;
            
            int i;
            static const int masks[] = {
                Z_Options_search, Z_Options_present, -1 
            };
            for (i = 0; masks[i] != -1; i++)
                if (ODR_MASK_GET(req->options, masks[i]))
                    ODR_MASK_SET(resp->options, masks[i]);
            if (m_p->m_support_named_result_sets)
            {
                if (ODR_MASK_GET(req->options, Z_Options_namedResultSets))
                    ODR_MASK_SET(resp->options, Z_Options_namedResultSets);
                else
                    m_p->m_support_named_result_sets = false;
            }
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

        }
        else if (apdu_req->which == Z_APDU_searchRequest)
        { 
            apdu_res = zget_APDU(odr, Z_APDU_searchResponse);
            Z_SearchRequest *req = apdu_req->u.searchRequest;
            Z_SearchResponse *resp = apdu_res->u.searchResponse;

            if (!m_p->m_support_named_result_sets && 
                strcmp(req->resultSetName, "default"))
            {
                Z_Records *rec = (Z_Records *)
                    odr_malloc(odr, sizeof(Z_Records));
                resp->records = rec;
                rec->which = Z_Records_NSD;
                rec->u.nonSurrogateDiagnostic =
                    zget_DefaultDiagFormat(
                        odr, YAZ_BIB1_RESULT_SET_NAMING_UNSUPP, 0);
            }
            else
                *resp->resultCount = 42;
        }
        else if (apdu_req->which == Z_APDU_presentRequest)
        { 
            apdu_res = zget_APDU(odr, Z_APDU_presentResponse);
        }
        else
        {
            apdu_res = zget_APDU(odr, Z_APDU_close);            
            *apdu_res->u.close->closeReason = Z_Close_protocolError;
            package.session().close();
        }
        if (apdu_res)
            package.response() = apdu_res;
        odr_destroy(odr);
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
