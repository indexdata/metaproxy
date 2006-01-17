/* $Id: filter_backend_test.cpp,v 1.15 2006-01-17 13:54:36 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"
#include "util.hpp"
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

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class Session_info {
            int dummy;
        };
        class Backend_test::Rep {
            friend class Backend_test;
            
        private:
            bool m_support_named_result_sets;

            session_map<Session_info> m_sessions;
        };
    }
}

using namespace yp2;

yf::Backend_test::Backend_test() : m_p(new Backend_test::Rep) {
    m_p->m_support_named_result_sets = false;
}

yf::Backend_test::~Backend_test() {
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
        yp2::odr odr;
        
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

            Session_info info;
            m_p->m_sessions.create(info, package.session());
        }
        else if (apdu_req->which == Z_APDU_searchRequest)
        {
            apdu_res = odr.create_searchResponse(apdu_req, 0, 0);
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
            apdu_res =
                odr.create_presentResponse(
                    apdu_req,
                    YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                    "backend_test: present not implemented");
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

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::Backend_test;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_backend_test = {
        0,
        "backend_test",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
