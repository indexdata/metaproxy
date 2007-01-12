/* $Id: filter_cql_to_rpn.cpp,v 1.1 2007-01-12 10:16:21 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include "util.hpp"
#include "filter_cql_to_rpn.hpp"

#include <yaz/log.h>
#include <yaz/cql.h>
#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>
#include <yaz/srw.h>
#include <yazpp/z-query.h>
#include <yazpp/cql2rpn.h>
#include <map>
#include <iostream>
#include <time.h>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class CQL_to_RPN::Rep {
            friend class CQL_to_RPN;

            Rep();

            yazpp_1::Yaz_cql2rpn cql2rpn;
        };
    }
}

yf::CQL_to_RPN::Rep::Rep()
{

}

yf::CQL_to_RPN::CQL_to_RPN() : m_p(new CQL_to_RPN::Rep)
{
}

yf::CQL_to_RPN::~CQL_to_RPN()
{

}

void yf::CQL_to_RPN::process(mp::Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_searchRequest)
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        Z_SearchRequest *sr = gdu->u.z3950->u.searchRequest;
        if (sr->query && sr->query->which == Z_Query_type_104 &&
            sr->query->u.type_104->which == Z_External_CQL)
        {
            char *addinfo = 0;
            Z_RPNQuery *rpnquery = 0;
            mp::odr odr;
            
            int r = m_p->cql2rpn.query_transform(sr->query->u.type_104->u.cql,
                                                 &rpnquery, odr,
                                                 &addinfo);
            if (r == -3)
            {
                yaz_log(YLOG_LOG, "No CQL to RPN table");
                Z_APDU *f_apdu = 
                    odr.create_searchResponse(
                        apdu_req, 
                        YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                        "Missing CQL to RPN spec");
                package.response() = f_apdu;
                return;
            }
            else if (r)
            {
                int error_code = yaz_diag_srw_to_bib1(r);

                yaz_log(YLOG_LOG, "CQL Conversion error %d", r);
                Z_APDU *f_apdu = 
                    odr.create_searchResponse(apdu_req, error_code, addinfo);
                package.response() = f_apdu;
                return;
            }
            else
            {   // conversion OK
                
                sr->query->which = Z_Query_type_1;
                sr->query->u.type_1 = rpnquery;
                package.request() = gdu;
            }
        }
    }
    package.move();
}

void yf::CQL_to_RPN::configure(const xmlNode *ptr)
{
    std::string fname;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "conversion"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "file"))
                    fname = mp::xml::get_text(attr);
                else
                    throw mp::filter::FilterException(
                        "Bad attribute " + std::string((const char *)
                                                       attr->name));
            }
        }
        else
        {
            throw mp::filter::FilterException("Bad element " 
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
    if (fname.length() == 0)
    {
        throw mp::filter::FilterException("Missing conversion spec for "
                                          "filter cql_to_rpn");
    }

    int error = 0;
    if (!m_p->cql2rpn.parse_spec_file(fname.c_str(), &error))
    {
        throw mp::filter::FilterException("Bad or missing CQL to RPN spec "
                                          + fname);
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::CQL_to_RPN;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_cql_to_rpn = {
        0,
        "cql_to_rpn",
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
