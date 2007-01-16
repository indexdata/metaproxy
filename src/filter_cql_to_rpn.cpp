/* $Id: filter_cql_to_rpn.cpp,v 1.5 2007-01-16 09:04:54 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "util.hpp"

#include "filter.hpp"
#include "package.hpp"

#include "filter_cql_to_rpn.hpp"

#include <yazpp/z-query.h>
#include <yaz/cql.h>
#include <yazpp/cql2rpn.h>
#include <yaz/zgdu.h>
#include <yaz/diagbib1.h>
#include <yaz/srw.h>


namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class CQLtoRPN::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
        private:
            yazpp_1::Yaz_cql2rpn m_cql2rpn;
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

void yf::CQLtoRPN::configure(const xmlNode *xmlnode)
{
    m_p->configure(xmlnode);
}

void yf::CQLtoRPN::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff

yf::CQLtoRPN::Impl::Impl()
{
}

yf::CQLtoRPN::Impl::~Impl()
{ 
}

void yf::CQLtoRPN::Impl::configure(const xmlNode *xmlnode)
{

    /*
      <filter type="cql_rpn">
      <conversion file="pqf.properties"/>
      </filter>
    */
    
    std::string fname;
    for (xmlnode = xmlnode->children; xmlnode; xmlnode = xmlnode->next)
    {
        if (xmlnode->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) xmlnode->name, "conversion"))
        {
            const struct _xmlAttr *attr;
            for (attr = xmlnode->properties; attr; attr = attr->next)
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
                                                             xmlnode->name));
        }
    }
    if (fname.length() == 0)
    {
        throw mp::filter::FilterException("Missing conversion configuration "
                                          "for filter cql_rpn");
    }

    int error = 0;
    if (!m_cql2rpn.parse_spec_file(fname.c_str(), &error))
    {
        throw mp::filter::FilterException("Bad or missing "
                                          "CQL to RPN configuration "
                                          + fname);
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
        if (sr->query && sr->query->which == Z_Query_type_104 &&
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
                        YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                        "Missing CQL to RPN configuration");
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
    struct metaproxy_1_filter_struct metaproxy_1_filter_cql_to_rpn = {
        0,
        "cql_rpn",
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
