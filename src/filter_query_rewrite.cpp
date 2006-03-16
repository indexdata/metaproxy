/* $Id: filter_query_rewrite.cpp,v 1.5 2006-03-16 09:38:33 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"

#include "util.hpp"
#include "filter_query_rewrite.hpp"

#include <yaz/zgdu.h>
#include <yaz/xmlquery.h>
#include <yaz/diagbib1.h>

#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class QueryRewrite::Rep {
        public:
            Rep();
            ~Rep();
            void process(yp2::Package &package) const;
            void configure(const xmlNode * ptr);
        private:
            xsltStylesheetPtr m_stylesheet;
        };
    }
}

yf::QueryRewrite::Rep::Rep()
{
    m_stylesheet = 0;
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

void yf::QueryRewrite::process(yp2::Package &package) const
{
    m_p->process(package);
}

void yp2::filter::QueryRewrite::configure(const xmlNode *ptr)
{
    m_p->configure(ptr);
}

void yf::QueryRewrite::Rep::process(yp2::Package &package) const
{
    Z_GDU *gdu = package.request().get();
    
    if (gdu && gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        if (apdu_req->which == Z_APDU_searchRequest)
        {
            int error_code = 0;
            const char *addinfo = 0;
            yp2::odr odr;
            Z_SearchRequest *req = apdu_req->u.searchRequest;
            
            xmlDocPtr doc_input = 0;
            yaz_query2xml(req->query, &doc_input);
            
            if (!doc_input)
            {
                error_code = YAZ_BIB1_MALFORMED_QUERY;
                addinfo = "converion from Query to XML failed";
            }
            else
            {
                if (m_stylesheet)
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
                }
                xmlFreeDoc(doc_input);
            }
            package.request() = gdu;
            if (error_code)
            {
                Z_APDU *f_apdu = 
                    odr.create_searchResponse(apdu_req, error_code, addinfo);
                package.response() = f_apdu;
                return;
            }
        } 
    }
    package.move();
}

void yp2::filter::QueryRewrite::Rep::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "xslt"))
        {
            if (m_stylesheet)
            {
                throw yp2::filter::FilterException
                    ("Only one xslt element allowed in query_rewrite filter");
            }

            std::string fname = yp2::xml::get_text(ptr);
            m_stylesheet = xsltParseStylesheetFile(BAD_CAST fname.c_str());
            if (!m_stylesheet)
            {
                throw yp2::filter::FilterException
                    ("Failed to read stylesheet " 
                     + fname
                     + " in query_rewrite filter");
            }
        }
        else
        {
            throw yp2::filter::FilterException
                ("Bad element " 
                 + std::string((const char *) ptr->name)
                 + " in query_rewrite filter");
        }
    }
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::QueryRewrite;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_query_rewrite = {
        0,
        "query-rewrite",
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
