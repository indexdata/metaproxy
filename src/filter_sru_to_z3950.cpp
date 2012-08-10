/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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

// make std::min actually work on Windows
#define NOMINMAX 1

#include "config.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include "gduutil.hpp"
#include "sru_util.hpp"
#include "filter_sru_to_z3950.hpp"

#include <yaz/zgdu.h>
#include <yaz/z-core.h>
#include <yaz/srw.h>
#include <yaz/pquery.h>
#include <yaz/oid_db.h>
#include <yaz/log.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <iostream>
#include <sstream>
#include <string>
/* #include <algorithm> */
#include <map>

namespace mp = metaproxy_1;
namespace mp_util = metaproxy_1::util;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class SRUtoZ3950::Frontend : boost::noncopyable {
            friend class Impl;
            bool m_in_use;
        public:
            Frontend();
            ~Frontend();
        };
        class SRUtoZ3950::Impl {
        public:
            void configure(const xmlNode *xmlnode);
            void process(metaproxy_1::Package &package);
        private:
            FrontendPtr get_frontend(mp::Package &package);
            void release_frontend(mp::Package &package);
            std::map<std::string, const xmlNode *> m_database_explain;

            typedef std::map<std::string, int> ActiveUrlMap;

            boost::mutex m_url_mutex;
            boost::condition m_cond_url_ready;
            ActiveUrlMap m_active_urls;


            boost::mutex m_mutex_session;
            boost::condition m_cond_session_ready;
            std::map<mp::Session, FrontendPtr> m_clients;            
        private:
            void sru(metaproxy_1::Package &package, Z_GDU *zgdu_req);
            int z3950_build_query(
                mp::odr &odr_en, Z_Query *z_query, 
                const Z_SRW_searchRetrieveRequest *req
                ) const;
            
            bool z3950_init_request(
                mp::Package &package, 
                mp::odr &odr_en,
                std::string zurl,
                Z_SRW_PDU *sru_pdu_res,
                const Z_SRW_PDU *sru_pdu_req
                ) const;

            bool z3950_close_request(mp::Package &package) const;

            bool z3950_search_request(
                mp::Package &package,
                mp::odr &odr_en,
                Z_SRW_PDU *sru_pdu_res,
                Z_SRW_searchRetrieveRequest const *sr_req,
                std::string zurl
                ) const;

            bool z3950_present_request(
                mp::Package &package,
                mp::odr &odr_en,
                Z_SRW_PDU *sru_pdu_res,
                Z_SRW_searchRetrieveRequest const *sr_req
                ) const;
            
            bool z3950_to_srw_diagnostics_ok(
                mp::odr &odr_en, 
                Z_SRW_searchRetrieveResponse *srw_res,
                Z_Records *records
                ) const;
            
            int z3950_to_srw_diag(
                mp::odr &odr_en, 
                Z_SRW_searchRetrieveResponse *srw_res,
                Z_DefaultDiagFormat *ddf
                ) const;

        };
    }
}

yf::SRUtoZ3950::SRUtoZ3950() : m_p(new Impl)
{
}

yf::SRUtoZ3950::~SRUtoZ3950()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::SRUtoZ3950::configure(const xmlNode *xmlnode, bool test_only,
                               const char *path)
{
    m_p->configure(xmlnode);
}

void yf::SRUtoZ3950::process(mp::Package &package) const
{
    m_p->process(package);
}

void yf::SRUtoZ3950::Impl::configure(const xmlNode *confignode)
{
    const xmlNode * dbnode;
    
    for (dbnode = confignode->children; dbnode; dbnode = dbnode->next)
    {
        if (dbnode->type != XML_ELEMENT_NODE)
            continue;
        
        std::string database;
        mp::xml::check_element_mp(dbnode, "database");

        for (struct _xmlAttr *attr = dbnode->properties; 
             attr; attr = attr->next)
        {
            
            mp::xml::check_attribute(attr, "", "name");
            database = mp::xml::get_text(attr);
             
            const xmlNode *explainnode;
            for (explainnode = dbnode->children; 
                 explainnode; explainnode = explainnode->next)
            {
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

void yf::SRUtoZ3950::Impl::sru(mp::Package &package, Z_GDU *zgdu_req)
{
    bool ok = true;    

    mp::odr odr_de(ODR_DECODE);
    Z_SRW_PDU *sru_pdu_req = 0;

    mp::odr odr_en(ODR_ENCODE);
    Z_SRW_PDU *sru_pdu_res = yaz_srw_get(odr_en, Z_SRW_explain_response);

    // determine database with the HTTP header information only
    mp_util::SRUServerInfo sruinfo = mp_util::get_sru_server_info(package);
    std::map<std::string, const xmlNode *>::iterator idbexp;
    idbexp = m_database_explain.find(sruinfo.database);

    // assign explain config XML DOM node if database is known
    const xmlNode *explainnode = 0;
    if (idbexp != m_database_explain.end())
    {
        explainnode = idbexp->second;
    }

    // decode SRU request
    Z_SOAP *soap = 0;
    char *charset = 0;
    char *stylesheet = 0;

    // filter acts as sink for non-valid SRU requests
    if (! (sru_pdu_req = mp_util::decode_sru_request(package, odr_de, odr_en, 
                                                     sru_pdu_res, &soap,
                                                     charset, stylesheet)))
    {
        if (soap)
        {
            mp_util::build_sru_explain(package, odr_en, sru_pdu_res, 
                                       sruinfo, explainnode);
            mp_util::build_sru_response(package, odr_en, soap, 
                                        sru_pdu_res, charset, stylesheet);
        }
        else
        {
            metaproxy_1::odr odr; 
            Z_GDU *zgdu_res = 
                odr.create_HTTP_Response(package.session(), 
                                         zgdu_req->u.HTTP_Request, 400);
            package.response() = zgdu_res;
        }
        return;
    }
    
    bool enable_package_log = false;
    std::string zurl;
    Z_SRW_extra_arg *arg;

    for ( arg = sru_pdu_req->extra_args; arg; arg = arg->next)
        if (!strcmp(arg->name, "x-target"))
        {
            zurl = std::string(arg->value);
        }
        else if (!strcmp(arg->name, "x-max-sockets"))
        {
            package.origin().set_max_sockets(atoi(arg->value));
        }
        else if (!strcmp(arg->name, "x-session-id"))
        {
            package.origin().set_custom_session(arg->value);
        }
        else if (!strcmp(arg->name, "x-log-enable"))
        {
            if (*arg->value == '1')
            {
                enable_package_log = true;
                package.log_enable();
            }
        }
    assert(sru_pdu_req);

    // filter acts as sink for SRU explain requests
    if (sru_pdu_req->which == Z_SRW_explain_request)
    {
        Z_SRW_explainRequest *er_req = sru_pdu_req->u.explain_request;
        mp_util::build_sru_explain(package, odr_en, sru_pdu_res, 
                                   sruinfo, explainnode, er_req);
    }
    else if (sru_pdu_req->which == Z_SRW_searchRetrieve_request
             && sru_pdu_req->u.request)
    {   // searchRetrieve
        Z_SRW_searchRetrieveRequest *sr_req = sru_pdu_req->u.request;   
        
        sru_pdu_res = yaz_srw_get_pdu(odr_en, Z_SRW_searchRetrieve_response,
                                      sru_pdu_req->srw_version);

        // checking that we have a query
        ok = mp_util::check_sru_query_exists(package, odr_en, 
                                             sru_pdu_res, sr_req);

        if (ok && z3950_init_request(package, odr_en,
                                     zurl, sru_pdu_res, sru_pdu_req))
        {
            ok = z3950_search_request(package, odr_en,
                                      sru_pdu_res, sr_req, zurl);
            
            if (ok 
                && sru_pdu_res->u.response->numberOfRecords
                && *(sru_pdu_res->u.response->numberOfRecords))

                ok = z3950_present_request(package, odr_en,
                                           sru_pdu_res,
                                           sr_req);
            z3950_close_request(package);
        }
    }

    // scan
    else if (sru_pdu_req->which == Z_SRW_scan_request
             && sru_pdu_req->u.scan_request)
    {
        sru_pdu_res = yaz_srw_get_pdu(odr_en, Z_SRW_scan_response,
                                      sru_pdu_req->srw_version);
        
        // we do not do scan at the moment, therefore issuing a diagnostic
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.scan_response->diagnostics), 
                               &(sru_pdu_res->u.scan_response->num_diagnostics), 
                               YAZ_SRW_UNSUPP_OPERATION, "scan");
    }
    else
    {
        sru_pdu_res = yaz_srw_get(odr_en, Z_SRW_explain_response);
        
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.explain_response->diagnostics), 
                               &(sru_pdu_res->u.explain_response->num_diagnostics), 
                               YAZ_SRW_UNSUPP_OPERATION, "unknown");
    }

    if (enable_package_log)
    {
        std::string l;
        package.log_reset(l);
        if (l.length())
        {
            mp::wrbuf w;
            
            wrbuf_puts(w, "<log>\n");
            wrbuf_xmlputs(w, l.c_str());
            wrbuf_puts(w, "</log>");
            
            sru_pdu_res->extraResponseData_len = w.len();
            sru_pdu_res->extraResponseData_buf =
                odr_strdup(odr_en, wrbuf_cstr(w));
        }
    }
    
    // build and send SRU response
    mp_util::build_sru_response(package, odr_en, soap, 
                                sru_pdu_res, charset, stylesheet);
}


yf::SRUtoZ3950::Frontend::Frontend() :  m_in_use(true)
{
}

yf::SRUtoZ3950::Frontend::~Frontend()
{
}


yf::SRUtoZ3950::FrontendPtr yf::SRUtoZ3950::Impl::get_frontend(
    mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex_session);

    std::map<mp::Session,yf::SRUtoZ3950::FrontendPtr>::iterator it;
    
    while (true)
    {
        it = m_clients.find(package.session());
        if (it == m_clients.end())
            break;
        
        if (!it->second->m_in_use)
        {
            it->second->m_in_use = true;
            return it->second;
        }
        m_cond_session_ready.wait(lock);
    }
    FrontendPtr f(new Frontend);
    m_clients[package.session()] = f;
    f->m_in_use = true;
    return f;
}

void yf::SRUtoZ3950::Impl::release_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex_session);
    std::map<mp::Session,FrontendPtr>::iterator it;
    
    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        if (package.session().is_closed())
        {
            m_clients.erase(it);
        }
        else
        {
            it->second->m_in_use = false;
        }
        m_cond_session_ready.notify_all();
    }
}

void yf::SRUtoZ3950::Impl::process(mp::Package &package)
{
    FrontendPtr f = get_frontend(package);

    Z_GDU *zgdu_req = package.request().get();

    if (zgdu_req && zgdu_req->which == Z_GDU_HTTP_Request)
    {
        if (zgdu_req->u.HTTP_Request->content_len == 0)
        {
            const char *path = zgdu_req->u.HTTP_Request->path;
            boost::mutex::scoped_lock lock(m_url_mutex);
            while (1)
            {
                ActiveUrlMap::iterator it = m_active_urls.find(path);
                if (it == m_active_urls.end())
                {
                    m_active_urls[path] = 1;
                    break;
                }
                yaz_log(YLOG_LOG, "Waiting for %s to complete", path);
                m_cond_url_ready.wait(lock);
            }
        }
        sru(package, zgdu_req);
        if (zgdu_req && zgdu_req->u.HTTP_Request->content_len == 0)
        {
            const char *path = zgdu_req->u.HTTP_Request->path;
            boost::mutex::scoped_lock lock(m_url_mutex);
            
            ActiveUrlMap::iterator it = m_active_urls.find(path);
            
            m_active_urls.erase(it);
            m_cond_url_ready.notify_all();
        }
    }
    else
    {
        package.move();
    }
    release_frontend(package);
}

bool 
yf::SRUtoZ3950::Impl::z3950_init_request(mp::Package &package, 
                                         mp::odr &odr_en,
                                         std::string zurl,
                                         Z_SRW_PDU *sru_pdu_res,
                                         const Z_SRW_PDU *sru_pdu_req) const
{
    // prepare Z3950 package
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package);

    // set initRequest APDU
    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_initRequest);
    Z_InitRequest *init_req = apdu->u.initRequest;

    Z_IdAuthentication *auth = NULL;
    if (sru_pdu_req->username && !sru_pdu_req->password)
    {
        auth = (Z_IdAuthentication *) odr_malloc(odr_en, sizeof(Z_IdAuthentication));
        auth->which = Z_IdAuthentication_open;
        auth->u.open = odr_strdup(odr_en, sru_pdu_req->username);
    }
    else if (sru_pdu_req->username && sru_pdu_req->password)
    {
        auth = (Z_IdAuthentication *) odr_malloc(odr_en, sizeof(Z_IdAuthentication));
        auth->which = Z_IdAuthentication_idPass;
        auth->u.idPass = (Z_IdPass *) odr_malloc(odr_en, sizeof(Z_IdPass));
        auth->u.idPass->groupId = NULL;
        auth->u.idPass->password = odr_strdup(odr_en, sru_pdu_req->password);
        auth->u.idPass->userId = odr_strdup(odr_en, sru_pdu_req->username);
    }

    init_req->idAuthentication = auth;

    *init_req->preferredMessageSize = 10*1024*1024;
    *init_req->maximumRecordSize = 10*1024*1024;
    
    ODR_MASK_SET(init_req->options, Z_Options_search);
    ODR_MASK_SET(init_req->options, Z_Options_present);
    ODR_MASK_SET(init_req->options, Z_Options_namedResultSets);
    ODR_MASK_SET(init_req->options, Z_Options_scan);

    ODR_MASK_SET(init_req->protocolVersion, Z_ProtocolVersion_1);
    ODR_MASK_SET(init_req->protocolVersion, Z_ProtocolVersion_2);
    ODR_MASK_SET(init_req->protocolVersion, Z_ProtocolVersion_3);

    if (zurl.length())
    {    
        std::string host;
        std::list<std::string> dblist;
        mp_util::split_zurl(zurl, host, dblist);
        mp_util::set_vhost_otherinfo(&init_req->otherInfo, odr_en, host, 1);
    }

    z3950_package.request() = apdu;

    // send Z3950 package
    z3950_package.move();

    // dead Z3950 backend detection
    if (z3950_package.session().is_closed())
    {
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_SYSTEM_TEMPORARILY_UNAVAILABLE, 0);
        return false;
    }

    // check successful initResponse
    Z_GDU *z3950_gdu = z3950_package.response().get();

    if (z3950_gdu && z3950_gdu->which == Z_GDU_Z3950 
        && z3950_gdu->u.z3950->which == Z_APDU_initResponse 
        && *z3950_gdu->u.z3950->u.initResponse->result)
        return true;
 
    yaz_add_srw_diagnostic(odr_en,
                           &(sru_pdu_res->u.response->diagnostics),
                           &(sru_pdu_res->u.response->num_diagnostics),
                           YAZ_SRW_SYSTEM_TEMPORARILY_UNAVAILABLE, 0);
    return false;
}

bool yf::SRUtoZ3950::Impl::z3950_close_request(mp::Package &package) const
{
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package);
    z3950_package.session().close();

    z3950_package.move();

    if (z3950_package.session().is_closed())
    {
        return true;
    }
    return false;
}

bool yf::SRUtoZ3950::Impl::z3950_search_request(mp::Package &package,  
                                                mp::odr &odr_en,
                                                Z_SRW_PDU *sru_pdu_res,
                                                Z_SRW_searchRetrieveRequest 
                                                const *sr_req,
                                                std::string zurl) const
{

    assert(sru_pdu_res->u.response);

    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package);

    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_searchRequest);
    Z_SearchRequest *z_searchRequest = apdu->u.searchRequest;

    // RecordSyntax will always be XML
    z_searchRequest->preferredRecordSyntax
        = odr_oiddup(odr_en, yaz_oid_recsyn_xml);

    if (!mp_util::set_databases_from_zurl(odr_en, zurl,
                                          &z_searchRequest->num_databaseNames,
                                          &z_searchRequest->databaseNames))
    {
        z_searchRequest->num_databaseNames = 1;
        z_searchRequest->databaseNames = (char**)
            odr_malloc(odr_en, sizeof(char *));

        if (sr_req->database)
            z_searchRequest->databaseNames[0] 
                = odr_strdup(odr_en, const_cast<char *>(sr_req->database));
        else
            z_searchRequest->databaseNames[0] 
                = odr_strdup(odr_en, "Default");
    }

    Z_Query *z_query = (Z_Query *) odr_malloc(odr_en, sizeof(Z_Query));
    z_searchRequest->query = z_query;
 
    int sru_diagnostic = z3950_build_query(odr_en, z_query, sr_req);
    if (sru_diagnostic)
    {    
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics), 
                               &(sru_pdu_res->u.response->num_diagnostics), 
                               sru_diagnostic,
                               "query");
        return false;
    }

    z3950_package.request() = apdu;
        
    z3950_package.move();

    Z_GDU *z3950_gdu = z3950_package.response().get();

    if (!z3950_gdu || z3950_gdu->which != Z_GDU_Z3950 
        || z3950_gdu->u.z3950->which != Z_APDU_searchResponse
        || !z3950_gdu->u.z3950->u.searchResponse
        || !z3950_gdu->u.z3950->u.searchResponse->searchStatus)
    {
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_SYSTEM_TEMPORARILY_UNAVAILABLE, 0);
        return false;
    }
    
    Z_SearchResponse *sr = z3950_gdu->u.z3950->u.searchResponse;

    if (!z3950_to_srw_diagnostics_ok(odr_en, sru_pdu_res->u.response, 
                                     sr->records))
    {
        return false;
    }

    sru_pdu_res->u.response->numberOfRecords
        = odr_intdup(odr_en, *sr->resultCount);
    return true;
}

bool yf::SRUtoZ3950::Impl::z3950_present_request(
    mp::Package &package, 
    mp::odr &odr_en,
    Z_SRW_PDU *sru_pdu_res,
    const Z_SRW_searchRetrieveRequest *sr_req)
    const
{
    assert(sru_pdu_res->u.response);
    int start = 1;
    int max_recs = 0;

    if (!sr_req)
        return false;

    if (sr_req->maximumRecords)
        max_recs = *sr_req->maximumRecords;
    if (sr_req->startRecord)
        start = *sr_req->startRecord;

    // no need to work if nobody wants record ..
    if (max_recs == 0)
        return true;

    bool send_z3950_present = true;

    // recordXPath unsupported.
    if (sr_req->recordXPath)
    {
        send_z3950_present = false;
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics), 
                               &(sru_pdu_res->u.response->num_diagnostics), 
                               YAZ_SRW_XPATH_RETRIEVAL_UNSUPP, 0);
    }
    
    // resultSetTTL unsupported.
    // resultSetIdleTime in response
    if (sr_req->resultSetTTL)
    {
        send_z3950_present = false;
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics), 
                               &(sru_pdu_res->u.response->num_diagnostics), 
                               YAZ_SRW_RESULT_SETS_UNSUPP, 0);
    }
    
    // sort unsupported
    if (sr_req->sort_type != Z_SRW_sort_type_none)
    {
        send_z3950_present = false;
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics), 
                               &(sru_pdu_res->u.response->num_diagnostics), 
                               YAZ_SRW_SORT_UNSUPP, 0);
    }
    
    // start record requested negative, or larger than number of records
    if (start < 0 || start > *sru_pdu_res->u.response->numberOfRecords)
    {
        send_z3950_present = false;
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics), 
                               &(sru_pdu_res->u.response->num_diagnostics), 
                               YAZ_SRW_FIRST_RECORD_POSITION_OUT_OF_RANGE, 0);
    }    
    
    // maximumRecords requested negative
    if (max_recs < 0)
    {
        send_z3950_present = false;
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics), 
                               &(sru_pdu_res->u.response->num_diagnostics), 
                               YAZ_SRW_UNSUPP_PARAMETER_VALUE,
                               "maximumRecords");
    }    

    // exit on all these above diagnostics
    if (!send_z3950_present)
        return false;
    
    if (max_recs > *sru_pdu_res->u.response->numberOfRecords - start)
        max_recs = *sru_pdu_res->u.response->numberOfRecords - start + 1;

    Z_SRW_searchRetrieveResponse *sru_res = sru_pdu_res->u.response;
    sru_res->records = (Z_SRW_record *)
        odr_malloc(odr_en, max_recs * sizeof(Z_SRW_record));
    int num = 0;
    while (num < max_recs)
    {
        // now packaging the z3950 present request
        Package z3950_package(package.session(), package.origin());
        z3950_package.copy_filter(package); 
        Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_presentRequest);
        
        assert(apdu->u.presentRequest);
        
        *apdu->u.presentRequest->resultSetStartPoint = start + num;
        *apdu->u.presentRequest->numberOfRecordsRequested = max_recs - num;
        
        // set response packing to be same as "request" packing..
        int record_packing = Z_SRW_recordPacking_XML;
        if (sr_req->recordPacking && 's' == *(sr_req->recordPacking))
            record_packing = Z_SRW_recordPacking_string;
        
        // RecordSyntax will always be XML
        apdu->u.presentRequest->preferredRecordSyntax
            = odr_oiddup(odr_en, yaz_oid_recsyn_xml);
        
        // z3950'fy record schema
        if (sr_req->recordSchema)
        {
            apdu->u.presentRequest->recordComposition 
                = (Z_RecordComposition *) 
                odr_malloc(odr_en, sizeof(Z_RecordComposition));
            apdu->u.presentRequest->recordComposition->which 
                = Z_RecordComp_simple;
            apdu->u.presentRequest->recordComposition->u.simple 
                = mp_util::build_esn_from_schema(odr_en,
                                                 (const char *) 
                                                 sr_req->recordSchema); 
        }
        
        // attaching Z3950 package to filter chain
        z3950_package.request() = apdu;
        
        // sending Z30.50 present request 
        z3950_package.move();
        
        //check successful Z3950 present response
        Z_GDU *z3950_gdu = z3950_package.response().get();
        if (!z3950_gdu || z3950_gdu->which != Z_GDU_Z3950 
            || z3950_gdu->u.z3950->which != Z_APDU_presentResponse
            || !z3950_gdu->u.z3950->u.presentResponse)
            
        {
            yaz_add_srw_diagnostic(odr_en,
                                   &(sru_pdu_res->u.response->diagnostics), 
                                   &(sru_pdu_res->u.response->num_diagnostics), 
                                   YAZ_SRW_SYSTEM_TEMPORARILY_UNAVAILABLE, 0);
            return false;
        }
        // everything fine, continuing
        
        Z_PresentResponse *pr = z3950_gdu->u.z3950->u.presentResponse;
        
        // checking non surrogate diagnostics in Z3950 present response package
        if (!z3950_to_srw_diagnostics_ok(odr_en, sru_pdu_res->u.response, 
                                         pr->records))
            return false;
        
        // if anything but database or surrogate diagnostics, stop
        if (!pr->records || pr->records->which != Z_Records_DBOSD)
            break;
        else
        {
            // inserting all records
            int returned_recs =
                pr->records->u.databaseOrSurDiagnostics->num_records;
            for (int i = 0; i < returned_recs; i++)
            {
                int position = i + *apdu->u.presentRequest->resultSetStartPoint;
                Z_NamePlusRecord *npr 
                    = pr->records->u.databaseOrSurDiagnostics->records[i];
                
                sru_res->records[i + num].recordPacking = record_packing;
                
                if (npr->which == Z_NamePlusRecord_databaseRecord &&
                    npr->u.databaseRecord->direct_reference 
                    && !oid_oidcmp(npr->u.databaseRecord->direct_reference,
                                   yaz_oid_recsyn_xml))
                {
                    // got XML record back
                    Z_External *r = npr->u.databaseRecord;
                    sru_res->records[i + num].recordPosition = 
                        odr_intdup(odr_en, position);
                    sru_res->records[i + num].recordSchema = sr_req->recordSchema;
                    sru_res->records[i + num].recordData_buf
                        = odr_strdupn(odr_en, 
                                      (const char *)r->u.octet_aligned->buf, 
                                      r->u.octet_aligned->len);
                    sru_res->records[i + num].recordData_len 
                        = r->u.octet_aligned->len;
                }
                else
                {
                    // not XML or no database record at all
                    yaz_mk_sru_surrogate(
                        odr_en, sru_res->records + i + num, position,
                        YAZ_SRW_RECORD_NOT_AVAILABLE_IN_THIS_SCHEMA, 0);
                }
            }
            num += returned_recs;
        }
    }
    sru_res->num_records = num;
    if (start - 1 + num < *sru_pdu_res->u.response->numberOfRecords)
        sru_res->nextRecordPosition =
            odr_intdup(odr_en, start + num);
    return true;
}

int yf::SRUtoZ3950::Impl::z3950_build_query(
    mp::odr &odr_en, Z_Query *z_query, 
    const Z_SRW_searchRetrieveRequest *req
    ) const
{        
    if (req->query_type == Z_SRW_query_type_cql)
    {
        Z_External *ext = (Z_External *) 
            odr_malloc(odr_en, sizeof(*ext));
        ext->direct_reference = 
            odr_getoidbystr(odr_en, "1.2.840.10003.16.2");
        ext->indirect_reference = 0;
        ext->descriptor = 0;
        ext->which = Z_External_CQL;
        ext->u.cql = odr_strdup(odr_en, req->query.cql);
        
        z_query->which = Z_Query_type_104;
        z_query->u.type_104 =  ext;
        return 0;
    }

    if (req->query_type == Z_SRW_query_type_pqf)
    {
        Z_RPNQuery *RPNquery;
        YAZ_PQF_Parser pqf_parser;
        
        pqf_parser = yaz_pqf_create ();
        
        RPNquery = yaz_pqf_parse (pqf_parser, odr_en, req->query.pqf);

        yaz_pqf_destroy(pqf_parser);

        if (!RPNquery)
            return YAZ_SRW_QUERY_SYNTAX_ERROR;

        z_query->which = Z_Query_type_1;
        z_query->u.type_1 =  RPNquery;
        
        return 0;
    }
    return YAZ_SRW_MANDATORY_PARAMETER_NOT_SUPPLIED;
}

bool yf::SRUtoZ3950::Impl::z3950_to_srw_diagnostics_ok(
    mp::odr &odr_en, 
    Z_SRW_searchRetrieveResponse 
    *sru_res,
    Z_Records *records) const
{
    // checking non surrogate diagnostics in Z3950 present response package
    if (records 
        && records->which == Z_Records_NSD
        && records->u.nonSurrogateDiagnostic)
    {
        z3950_to_srw_diag(odr_en, sru_res, 
                          records->u.nonSurrogateDiagnostic);
        return false;
    }
    return true;
}

int yf::SRUtoZ3950::Impl::z3950_to_srw_diag(
    mp::odr &odr_en, 
    Z_SRW_searchRetrieveResponse *sru_res,
    Z_DefaultDiagFormat *ddf) const
{
    int bib1_code = *ddf->condition;
    sru_res->num_diagnostics = 1;
    sru_res->diagnostics = (Z_SRW_diagnostic *)
        odr_malloc(odr_en, sizeof(*sru_res->diagnostics));
    yaz_mk_std_diagnostic(odr_en, sru_res->diagnostics,
                          yaz_diag_bib1_to_srw(bib1_code), 
                          ddf->u.v2Addinfo);
    return 0;
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::SRUtoZ3950;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_sru_to_z3950 = {
        0,
        "sru_z3950",
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

