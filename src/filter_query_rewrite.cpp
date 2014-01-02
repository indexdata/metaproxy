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
#include "filter_query_rewrite.hpp"

#include <yaz/log.h>
#include <yaz/zgdu.h>
#include <yaz/xmlquery.h>
#include <yaz/diagbib1.h>
#include <yaz/query-charset.h>
#include <yaz/tpath.h>

#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class QueryRewrite::Rep {
        public:
            Rep();
            ~Rep();
            void process(mp::Package &package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        private:
            xsltStylesheetPtr m_stylesheet;
            std::string charset_from;
            std::string charset_to;
       };
    }
}

yf::QueryRewrite::Rep::Rep() : m_stylesheet(0), charset_from("UTF-8")
{
}

yf::QueryRewrite::Rep::~Rep()
{
    if (m_stylesheet)
        xsltFreeStylesheet(m_stylesheet);
}

yf::QueryRewrite::QueryRewrite() : m_p(new Rep)
{
}

yf::QueryRewrite::~QueryRewrite()
{  // must have a destructor because of boost::scoped_ptr
}

void mp::filter::QueryRewrite::configure(const xmlNode *ptr, bool test_only,
                                         const char *path)
{
    m_p->configure(ptr, test_only, path);
}

void yf::QueryRewrite::process(mp::Package &package) const
{
    m_p->process(package);
}

void yf::QueryRewrite::Rep::process(mp::Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (gdu && gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        if (apdu_req->which == Z_APDU_searchRequest)
        {
            int error_code = 0;
            const char *addinfo = 0;
            mp::odr odr;
            Z_SearchRequest *req = apdu_req->u.searchRequest;

            if (m_stylesheet)
            {
                xmlDocPtr doc_input = 0;
                yaz_query2xml(req->query, &doc_input);

                if (!doc_input)
                {
                    error_code = YAZ_BIB1_MALFORMED_QUERY;
                    addinfo = "converion from Query to XML failed";
                }
                else
                {
                    xmlDocPtr doc_res = xsltApplyStylesheet(m_stylesheet,
                                                            doc_input, 0);
                    if (!doc_res)
                    {
                        error_code = YAZ_BIB1_MALFORMED_QUERY;
                        addinfo = "XSLT transform failed for query";
                    }
                    else
                    {
                        const xmlNode *root_element = xmlDocGetRootElement(doc_res);
                        yaz_xml2query(root_element, &req->query, odr,
                                      &error_code, &addinfo);
                        xmlFreeDoc(doc_res);
                    }
                    xmlFreeDoc(doc_input);
                }
            }
            if (!error_code && charset_to.length() && charset_from.length() &&
                (req->query->which == Z_Query_type_1
                 || req->query->which == Z_Query_type_101))
            {
                yaz_iconv_t cd = yaz_iconv_open(charset_to.c_str(),
                                                charset_from.c_str());
                if (cd)
                {
                    int r = yaz_query_charset_convert_rpnquery_check(
                        req->query->u.type_1, odr, cd);
                    yaz_iconv_close(cd);
                    if (r)
                    {  /* query could not be char converted */
                        error_code = YAZ_BIB1_MALFORMED_QUERY;
                        addinfo = "could not convert query to target charset";
                    }
                }
            }
            if (error_code)
            {
                Z_APDU *f_apdu =
                    odr.create_searchResponse(apdu_req, error_code, addinfo);
                package.response() = f_apdu;
                return;
            }
            package.request() = gdu;
        }
    }
    package.move();
}

void mp::filter::QueryRewrite::Rep::configure(const xmlNode *ptr,
                                              bool test_only, const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;

        if (mp::xml::is_element_mp(ptr, "xslt"))
        {
            if (m_stylesheet)
            {
                throw mp::filter::FilterException
                    ("Only one xslt element allowed in query_rewrite filter");
            }

            std::string fname;

            for (struct _xmlAttr *attr = ptr->properties;
                 attr; attr = attr->next)
            {
                mp::xml::check_attribute(attr, "", "stylesheet");
                fname = mp::xml::get_text(attr);
            }

            if (0 == fname.size())
                throw mp::filter::FilterException
                    ("Attribute <xslt stylesheet=\""
                     + fname
                     + "\"> needs XSLT stylesheet path content"
                     + " in query_rewrite filter");

            char fullpath[1024];
            char *cp = yaz_filepath_resolve(fname.c_str(), path, 0, fullpath);
            if (!cp)
            {
                throw mp::filter::FilterException("Cannot read XSLT " + fname);
            }

            m_stylesheet = xsltParseStylesheetFile(BAD_CAST cp);
            if (!m_stylesheet)
            {
                throw mp::filter::FilterException
                    ("Failed to read XSLT stylesheet '"
                     + fname
                     + "' in query_rewrite filter");
            }
        }
        else if (mp::xml::is_element_mp(ptr, "charset"))
        {
            for (struct _xmlAttr *attr = ptr->properties;
                 attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "from"))
                {
                    charset_from = mp::xml::get_text(attr);
                }
                else if (!strcmp((const char *) attr->name, "to"))
                {
                    charset_to = mp::xml::get_text(attr);
                }
                else
                    throw mp::filter::FilterException
                        ("Invalid attribute inside charset inside "
                         "query_rewrite filter");
            }
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element "
                 + std::string((const char *) ptr->name)
                 + " in query_rewrite filter");
        }
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::QueryRewrite;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_query_rewrite = {
        0,
        "query_rewrite",
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

