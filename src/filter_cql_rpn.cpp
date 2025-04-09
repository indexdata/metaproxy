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
#include <metaproxy/util.hpp>

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>

#include "filter_cql_rpn.hpp"

#include <yazpp/z-query.h>
#include <yaz/cql.h>
#include <yazpp/cql2rpn.h>
#include <yaz/zgdu.h>
#include <yaz/diagbib1.h>
#include <yaz/srw.h>
#include <yaz/tpath.h>
#include <yaz/oid_std.h>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class CQLtoRPN::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode *ptr, const char *path);
        private:
            yazpp_1::Yaz_cql2rpn m_cql2rpn;
            bool reverse;
        };
    }
}


// define Pimpl wrapper forwarding to Impl

yf::CQLtoRPN::CQLtoRPN() : m_p(new Impl)
{
}

yf::CQLtoRPN::~CQLtoRPN()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::CQLtoRPN::configure(const xmlNode *xmlnode, bool test_only,
                             const char *path)
{
    m_p->configure(xmlnode, path);
}

void yf::CQLtoRPN::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff

yf::CQLtoRPN::Impl::Impl() : reverse(false)
{
}

yf::CQLtoRPN::Impl::~Impl()
{
}

void yf::CQLtoRPN::Impl::configure(const xmlNode *xmlnode, const char *path)
{
    int no_conversions = 0;
    for (const xmlNode *node = xmlnode->children; node; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp((const char *) node->name, "conversion"))
        {
            throw mp::filter::FilterException("Bad element "
                + std::string((const char *)
                              node->name));
        }
        const struct _xmlAttr *attr;
        for (attr = node->properties; attr; attr = attr->next)
        {
            if (!strcmp((const char *) attr->name, "file"))
            {
                std::string fname = mp::xml::get_text(attr);
                char fullpath[1024];
                if (!yaz_filepath_resolve(fname.c_str(), path, 0, fullpath))
                    throw mp::filter::FilterException("Could not open " + fname);
                int error = 0;
                if (!m_cql2rpn.parse_spec_file(fullpath, &error))
                {
                    throw mp::filter::FilterException("Bad or missing "
                                                    "CQL to RPN configuration "
                                                    + fname);
                }
                no_conversions++;
            }
            else if (!strcmp((const char *) attr->name, "key"))
            {
                std::string key = mp::xml::get_text(attr);
                std::string val = mp::xml::get_text(node);
                if (m_cql2rpn.define_pattern(key.c_str(), val.c_str()))
                    throw mp::filter::FilterException(
                        "Bad CQL to RPN pattern: " + key + "=" + val);
                no_conversions++;
            }
            else if (!strcmp((const char *) attr->name, "reverse"))
            {
                reverse = mp::xml::get_bool(attr->children, 0);
            }
            else
                throw mp::filter::FilterException(
                    "Bad attribute " + std::string((const char *)
                                                    attr->name));
        }
    }
    if (no_conversions == 0)
    {
        throw mp::filter::FilterException("Missing conversion configuration "
                                          "for filter cql_rpn");
    }
}

void yf::CQLtoRPN::Impl::process(mp::Package &package)
{
    Z_GDU *gdu = package.request().get();

    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_searchRequest)
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        Z_SearchRequest *sr = gdu->u.z3950->u.searchRequest;
        if (reverse && sr->query && sr->query->which == Z_Query_type_1)
        {
            char *addinfo = 0;
            mp::odr odr;
            WRBUF cql = wrbuf_alloc();

            int r = m_cql2rpn.rpn2cql_transform(sr->query->u.type_1, cql,
                                                odr, &addinfo);
            if (r)
            {
                Z_APDU *f_apdu =
                    odr.create_searchResponse(apdu_req, r, addinfo);
                package.response() = f_apdu;
                return;
            }
            else
            {
                Z_External *ext = (Z_External *)
                    odr_malloc(odr, sizeof(*ext));
                ext->direct_reference = odr_oiddup(odr,
                                                   yaz_oid_userinfo_cql);
                ext->indirect_reference = 0;
                ext->descriptor = 0;
                ext->which = Z_External_CQL;
                ext->u.cql = odr_strdup(odr, wrbuf_cstr(cql));

                sr->query->which = Z_Query_type_104;
                sr->query->u.type_104 = ext;

                package.request() = gdu;
            }
            wrbuf_destroy(cql);
        }
        if (!reverse && sr->query && sr->query->which == Z_Query_type_104 &&
            sr->query->u.type_104->which == Z_External_CQL)
        {
            char *addinfo = 0;
            Z_RPNQuery *rpnquery = 0;
            mp::odr odr;

            int r = m_cql2rpn.query_transform(sr->query->u.type_104->u.cql,
                                                 &rpnquery, odr,
                                                 &addinfo);
            if (r == -3)
            {
                Z_APDU *f_apdu =
                    odr.create_searchResponse(
                        apdu_req,
                        YAZ_BIB1_PERMANENT_SYSTEM_ERROR,
                        "cql_rpn: missing CQL to RPN configuration");
                package.response() = f_apdu;
                return;
            }
            else if (r)
            {
                int error_code = yaz_diag_srw_to_bib1(r);

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


static mp::filter::Base* filter_creator()
{
    return new mp::filter::CQLtoRPN;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_cql_rpn = {
        0,
        "cql_rpn",
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

