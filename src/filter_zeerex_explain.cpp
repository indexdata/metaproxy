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
#include "filter_zeerex_explain.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include "gduutil.hpp"
#include "sru_util.hpp"

#include <yaz/zgdu.h>
#include <yaz/z-core.h>
#include <yaz/srw.h>
#include <yaz/pquery.h>

#include <boost/thread/mutex.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>

namespace mp = metaproxy_1;
namespace mp_util = metaproxy_1::util;
namespace yf = mp::filter;


namespace metaproxy_1 {
    namespace filter {
        class ZeeRexExplain::Impl {
        public:
            void configure(const xmlNode *xmlnode);
            void process(metaproxy_1::Package &package);
        private:
            std::map<std::string, const xmlNode *> m_database_explain;
        };
    }
}

yf::ZeeRexExplain::ZeeRexExplain() : m_p(new Impl)
{
}

yf::ZeeRexExplain::~ZeeRexExplain()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::ZeeRexExplain::configure(const xmlNode *xmlnode, bool test_only,
                                  const char *path)
{
    m_p->configure(xmlnode);
}

void yf::ZeeRexExplain::process(mp::Package &package) const
{
    m_p->process(package);
}

void yf::ZeeRexExplain::Impl::configure(const xmlNode *confignode)
{
    const xmlNode * dbnode;

    for (dbnode = confignode->children; dbnode; dbnode = dbnode->next){
        if (dbnode->type != XML_ELEMENT_NODE)
            continue;

        std::string database;
        mp::xml::check_element_mp(dbnode, "database");

        for (struct _xmlAttr *attr = dbnode->properties;
             attr; attr = attr->next){

            mp::xml::check_attribute(attr, "", "name");
            database = mp::xml::get_text(attr);

            std::cout << database << "\n";

            const xmlNode *explainnode;
            for (explainnode = dbnode->children;
                 explainnode; explainnode = explainnode->next){
                if (explainnode->type != XML_ELEMENT_NODE)
                    continue;
                if (explainnode)
                    break;
            }
            // assigning explain node to database name - no check yet
            m_database_explain.insert(std::make_pair(database, explainnode));
         }
    }
}


void yf::ZeeRexExplain::Impl::process(mp::Package &package)
{
    Z_GDU *zgdu_req = package.request().get();

    // ignoring all non HTTP_Request packages
    if (!zgdu_req || !(zgdu_req->which == Z_GDU_HTTP_Request)){
        package.move();
        return;
    }

    // only working on  HTTP_Request packages now

    mp::odr odr_de(ODR_DECODE);
    Z_SRW_PDU *sru_pdu_req = 0;

    mp::odr odr_en(ODR_ENCODE);
    //Z_SRW_PDU *sru_pdu_res = 0;
    Z_SRW_PDU *sru_pdu_res = yaz_srw_get(odr_en, Z_SRW_explain_response);

    // finding correct SRU database and explain XML node fragment from config
    mp_util::SRUServerInfo sruinfo = mp_util::get_sru_server_info(package);

    const xmlNode *explainnode = 0;
    std::map<std::string, const xmlNode *>::iterator idbexp;
    idbexp = m_database_explain.find(sruinfo.database);

    //std::cout << "Finding " << sruinfo.database << "\n";
    if (idbexp != m_database_explain.end()){
        //std::cout << "Found " << idbexp->first << " " << idbexp->second << "\n";
        explainnode = idbexp->second;
    }
    else {
        // need to emmit error ?? or just let package pass ??
        //std::cout << "Missed " << sruinfo.database << "\n";
        package.move();
        return;
    }


    // if SRU package could not be decoded, send minimal explain and
    // close connection

    Z_SOAP *soap = 0;
    char *charset = 0;
    char *stylesheet = 0;
    if (! (sru_pdu_req = mp_util::decode_sru_request(package, odr_de, odr_en,
                                            sru_pdu_res, &soap,
                                            charset)))
    {
        mp_util::build_sru_explain(package, odr_en, sru_pdu_res,
                                   sruinfo, explainnode);
        mp_util::build_sru_response(package, odr_en, soap,
                           sru_pdu_res, charset, stylesheet);
        package.session().close();
        return;
    }


    if (sru_pdu_req->which != Z_SRW_explain_request)
    {
    // Let pass all other SRU actions
        package.move();
        return;
    }
    // except valid SRU explain request, construct ZeeRex Explain response
    else
    {
        Z_SRW_explainRequest *er_req = sru_pdu_req->u.explain_request;
        //mp_util::build_simple_explain(package, odr_en, sru_pdu_res,
        //                           sruinfo, er_req);
        mp_util::build_sru_explain(package, odr_en, sru_pdu_res,
                                   sruinfo, explainnode, er_req);
        mp_util::build_sru_response(package, odr_en, soap,
                                    sru_pdu_res, charset, stylesheet);
        return;
    }

    // should never arrive here
    package.session().close();
    return;
}



static mp::filter::Base* filter_creator()
{
    return new mp::filter::ZeeRexExplain;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_zeerex_explain = {
        0,
        "zeerex_explain",
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

