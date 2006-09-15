/* $Id: filter_sru_to_z3950.cpp,v 1.7 2006-09-15 14:18:25 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"
#include "util.hpp"
#include "gduutil.hpp"
#include "filter_sru_to_z3950.hpp"

#include <yaz/zgdu.h>
#include <yaz/srw.h>
#include <yaz/pquery.h>

#include <boost/thread/mutex.hpp>

#include <iostream>
#include <sstream>
#include <string>


namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 
{

    template<typename T>
    std::string to_string(const T& t)
    {
        std::ostringstream o;
        if(o << t)
            return o.str();
        
        return std::string();
    }

    std::string http_header_value(const Z_HTTP_Header* header, 
                                  const std::string name)
    {
        while (header && header->name
               && std::string(header->name) !=  name)
            header = header->next;
        
        if (header && header->name && std::string(header->name) == name
            && header->value)
            return std::string(header->value);

        return std::string();
    }
    

}


namespace metaproxy_1 {
    namespace filter {
        class SRUtoZ3950::Rep {
            //friend class SRUtoZ3950;
        public:
            void configure(const xmlNode *xmlnode);
            void process(metaproxy_1::Package &package) const;
        private:
            std::string sru_protocol(const Z_HTTP_Request &http_req) const;
            std::string debug_http(const Z_HTTP_Request &http_req) const;
            void http_response(mp::Package &package, 
                               const std::string &content, 
                               int http_code = 200) const;
            bool build_sru_debug_package(mp::Package &package) const;
            bool z3950_build_query(mp::odr &odr_en, Z_Query *z_query, 
                                   const std::string &query, 
                                   int query_type) const;
            bool z3950_init_request(mp::Package &package, 
                                         const std::string 
                                         &database = "Default") const;
            bool z3950_close_request(mp::Package &package) const;
            bool z3950_search_request(mp::Package &package,
                                      unsigned int &hits,
                                      const std::string &database, 
                                      const std::string &query, 
                                      int query_type) const;
            bool z3950_present_request(mp::Package &package) const;
            bool z3950_scan_request(mp::Package &package) const;
        };
    }
}

yf::SRUtoZ3950::SRUtoZ3950() : m_p(new Rep)
{
}

yf::SRUtoZ3950::~SRUtoZ3950()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::SRUtoZ3950::configure(const xmlNode *xmlnode)
{
    m_p->configure(xmlnode);
}

void yf::SRUtoZ3950::process(mp::Package &package) const
{
    m_p->process(package);
}

void yf::SRUtoZ3950::Rep::configure(const xmlNode *xmlnode)
{
}

void yf::SRUtoZ3950::Rep::process(mp::Package &package) const
{
    Z_GDU *zgdu_req = package.request().get();

    // ignoring all non HTTP_Request packages
    if (!zgdu_req || !(zgdu_req->which == Z_GDU_HTTP_Request)){
        package.move();
        return;
    }
    
    // only working on  HTTP_Request packages now


    // TODO: Z3950 response parsing and translation to SRU package
    Z_HTTP_Request* http_req =  zgdu_req->u.HTTP_Request;
    bool ok;
    unsigned int search_hits = 0;

    if (z3950_init_request(package))
    {
        ok = z3950_search_request(package, search_hits, "Default", 
                                  "dc.title=bacillus", Z_SRW_query_type_cql);
        if (ok && search_hits)
            ok = z3950_present_request(package);
        //z3950_scan_request(package,"Default", 
        //                     "dc:title=bacillus", Z_SRW_query_type_cql);
        z3950_close_request(package);
    }
    
    build_sru_debug_package(package);
    return;








    // SRU request package checking
 

    //Z_GDU *zgdu_res = 0; 
    Z_SRW_PDU *sru_pdu_req = 0;
    //Z_SRW_PDU *sru_pdu_res = 0;
    //Z_APDU *z3950_apdu_req = 0;
    //Z_APDU *z3950_apdu_res = 0;
    

    Z_SOAP *soap_req = 0;
    char *charset = 0;
    Z_SRW_diagnostic *diag = 0;
    int num_diags = 0;
    //mp::odr odr_de(ODR_DECODE);
    
    if (0 == yaz_sru_decode(http_req, &sru_pdu_req, &soap_req, 
                            odr(ODR_DECODE), &charset, &diag, &num_diags))
    {
        std::cout << "SRU GET/POST \n";
    }
    else if (0 == yaz_srw_decode(http_req, &sru_pdu_req, &soap_req, 
                                 odr(ODR_DECODE), &charset))
    {
        std::cout << "SRU SOAP \n";
    } 
    else 
    {
        std::cout << "SRU DECODING ERROR - SHOULD NEVER HAPPEN\n";
        package.session().close();
        return;
    }

    if (num_diags)
    {
        std::cout << "SRU DIAGNOSTICS " << num_diags << "\n";
        // TODO: make nice diagnostic return package 
        //Z_SRW_PDU *srw_pdu_res =
        //            yaz_srw_get(odr(ODR_ENCODE),
        //                        Z_SRW_searchRetrieve_response);
        //        Z_SRW_searchRetrieveResponse *srw_res = srw_pdu_res->u.response;

        //        srw_res->diagnostics = diagnostic;
        //        srw_res->num_diagnostics = num_diagnostic;
        //        send_srw_response(srw_pdu_res);
        //        return;

        // package.session().close();
        return;
    }

    
    // SRU request package translation to Z3950 package

    // searchRetrieve
    if (sru_pdu_req && sru_pdu_req->which == Z_SRW_searchRetrieve_request)
    {
        //Z_SRW_searchRetrieveRequest *srw_req = sru_pdu_req->u.request;   

        // recordXPath unsupported.
        //if (srw_req->recordXPath)
        //    yaz_add_srw_diagnostic(odr_decode(),
        //                           &diag, &num_diags, 72, 0);
        // sort unsupported
        //    if (srw_req->sort_type != Z_SRW_sort_type_none)
        //        yaz_add_srw_diagnostic(odr_decode(),
        //                               &diag, &num_diags, 80, 0);
    }
    else
    {
        std::cout << "SRU OPERATION NOT SUPPORTED \n";
        // TODO: make nice diagnostic return package 
        // package.session().close();
        return;
    }

}

bool yf::SRUtoZ3950::Rep::build_sru_debug_package(mp::Package &package) const
{
    Z_GDU *zgdu_req = package.request().get();
    if  (zgdu_req && zgdu_req->which == Z_GDU_HTTP_Request)
    {    
        Z_HTTP_Request* http_req =  zgdu_req->u.HTTP_Request;
        std::string content = debug_http(*http_req);
        int http_code = 400;    
        http_response(package, content, http_code);
        return true;
    }
    return false;
}


bool 
yf::SRUtoZ3950::Rep::z3950_init_request(mp::Package &package, 
                                             const std::string &database) const
{
    // prepare Z3950 package
    //Session s;
    //Package z3950_package(s, package.origin());
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package);

    // set initRequest APDU
    mp::odr odr_en(ODR_ENCODE);
    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_initRequest);
    Z_InitRequest *init_req = apdu->u.initRequest;
    //TODO: add user name in apdu
    //TODO: add user passwd in apdu
    //init_req->idAuthentication = org_init->idAuthentication;
    //init_req->implementationId = "IDxyz";
    //init_req->implementationName = "NAMExyz";
    //init_req->implementationVersion = "VERSIONxyz";

    ODR_MASK_SET(init_req->options, Z_Options_search);
    ODR_MASK_SET(init_req->options, Z_Options_present);
    ODR_MASK_SET(init_req->options, Z_Options_namedResultSets);
    ODR_MASK_SET(init_req->options, Z_Options_scan);

    ODR_MASK_SET(init_req->protocolVersion, Z_ProtocolVersion_1);
    ODR_MASK_SET(init_req->protocolVersion, Z_ProtocolVersion_2);
    ODR_MASK_SET(init_req->protocolVersion, Z_ProtocolVersion_3);

    z3950_package.request() = apdu;

    // send Z3950 package
    // std::cout << "z3950_init_request " << *apdu <<"\n";
    z3950_package.move();

    // dead Z3950 backend detection
    if (z3950_package.session().is_closed()){
        package.session().close();
        return false;
    }

    // check successful initResponse
    Z_GDU *z3950_gdu = z3950_package.response().get();

    if (z3950_gdu && z3950_gdu->which == Z_GDU_Z3950 
        && z3950_gdu->u.z3950->which == Z_APDU_initResponse)
         return true;
 
    return false;
}

bool 
yf::SRUtoZ3950::Rep::z3950_close_request(mp::Package &package) const
{
    // prepare Z3950 package 
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package);
    z3950_package.session().close();

    // set close APDU
    //mp::odr odr_en(ODR_ENCODE);
    //Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_close);
    //z3950_package.request() = apdu;

    z3950_package.move();

    // check successful close response
    //Z_GDU *z3950_gdu = z3950_package.response().get();
    //if (z3950_gdu && z3950_gdu->which == Z_GDU_Z3950 
    //    && z3950_gdu->u.z3950->which == Z_APDU_close)
    //    return true;

    if (z3950_package.session().is_closed()){
        //package.session().close();
        return true;
    }
    return false;
}

bool 
yf::SRUtoZ3950::Rep::z3950_search_request(mp::Package &package,
                                          unsigned int &hits,
                                          const std::string &database, 
                                          const std::string &query, 
                                          int query_type) const
{
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package); 
    mp::odr odr_en(ODR_ENCODE);
    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_searchRequest);

    //TODO: add stuff in apdu
    Z_SearchRequest *z_searchRequest = apdu->u.searchRequest;
    z_searchRequest->num_databaseNames = 1;
    z_searchRequest->databaseNames = (char**)
        odr_malloc(odr_en, sizeof(char *));
    z_searchRequest->databaseNames[0] = odr_strdup(odr_en, database.c_str());

    // TODO query repackaging
    Z_Query *z_query = (Z_Query *) odr_malloc(odr_en, sizeof(Z_Query));
    z_searchRequest->query = z_query;

 
    if (!z3950_build_query(odr_en, z_query, query, query_type))
    {    
        //send_to_srw_client_error(7, "query");
        return false;
    }

    z3950_package.request() = apdu;
    //std::cout << "z3950_search_request " << *apdu << "\n";
        
    z3950_package.move();
    //TODO: check success condition
    Z_GDU *z3950_gdu = z3950_package.response().get();
    //std::cout << "z3950_search_request " << *z3950_gdu << "\n";

    if (z3950_gdu && z3950_gdu->which == Z_GDU_Z3950 
        && z3950_gdu->u.z3950->which == Z_APDU_searchResponse
        && z3950_gdu->u.z3950->u.searchResponse->searchStatus)
    {
        hits = *(z3950_gdu->u.z3950->u.searchResponse->resultCount);
        return true;
    }
    
    return false;
}

bool 
yf::SRUtoZ3950::Rep::z3950_present_request(mp::Package &package) const
{
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package); 
    mp::odr odr_en(ODR_ENCODE);
    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_presentRequest);
    //TODO: add stuff in apdu
    z3950_package.request() = apdu;

    //std::cout << "z3950_present_request " << *apdu << "\n";   
    z3950_package.move();

    //TODO: check success condition
    Z_GDU *z3950_gdu = z3950_package.response().get();
    if (z3950_gdu && z3950_gdu->which == Z_GDU_Z3950 
        && z3950_gdu->u.z3950->which == Z_APDU_presentResponse)
        //&& z3950_gdu->u.z3950->u.presenthResponse->searchStatus)
    {
    std::cout << "z3950_present_request OK\n";   
         return true;
    }

    return false;
}

bool 
yf::SRUtoZ3950::Rep::z3950_scan_request(mp::Package &package) const
{
    Package z3950_package(package.session(), package.origin());
    z3950_package.copy_filter(package); 
    mp::odr odr_en(ODR_ENCODE);
    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_scanRequest);
    //TODO: add stuff in apdu
    z3950_package.request() = apdu;

    std::cout << "z3950_scan_request " << *apdu << "\n";   
    z3950_package.move();
    //TODO: check success condition
    return true;
    return false;
}

bool yf::SRUtoZ3950::Rep::z3950_build_query(mp::odr &odr_en, Z_Query *z_query, 
                                            const std::string &query, 
                                            int query_type) const
{        
    if (query_type == Z_SRW_query_type_cql)
    {
        Z_External *ext = (Z_External *) 
            odr_malloc(odr_en, sizeof(*ext));
        ext->direct_reference = 
            odr_getoidbystr(odr_en, "1.2.840.10003.16.2");
        ext->indirect_reference = 0;
        ext->descriptor = 0;
        ext->which = Z_External_CQL;
        //ext->u.cql = srw_req->query.cql;
        ext->u.cql = const_cast<char *>(query.c_str());
        
        z_query->which = Z_Query_type_104;
        z_query->u.type_104 =  ext;
        return true;
    }

    if (query_type == Z_SRW_query_type_pqf)
    {
        Z_RPNQuery *RPNquery;
        YAZ_PQF_Parser pqf_parser;
        
        pqf_parser = yaz_pqf_create ();
        
        RPNquery = yaz_pqf_parse (pqf_parser, odr_en, query.c_str());
        //srw_req->query.pqf);
//                 if (!RPNquery)
//                 {
//                     const char *pqf_msg;
//                     size_t off;
//                     int code = yaz_pqf_error (pqf_parser, &pqf_msg, &off);
//                     int ioff = off;
//                     yaz_log(YLOG_LOG, "%*s^\n", ioff+4, "");
//                     yaz_log(YLOG_LOG, "Bad PQF: %s (code %d)\n", pqf_msg, code);
                    
//                     send_to_srw_client_error(10, 0);
//                     return;
//                 }
        z_query->which = Z_Query_type_1;
        z_query->u.type_1 =  RPNquery;
        
        yaz_pqf_destroy (pqf_parser);
        return true;
    }
    return false;
}


std::string 
yf::SRUtoZ3950::Rep::sru_protocol(const Z_HTTP_Request &http_req) const
{
    const std::string mime_urlencoded("application/x-www-form-urlencoded");
    const std::string mime_text_xml("text/xml");
    const std::string mime_soap_xml("application/soap+xml");

    const std::string http_method(http_req.method);
    const std::string http_type 
        =  http_header_value(http_req.headers, "Content-Type");

    if (http_method == "GET")
        return "SRU GET";

    if (http_method == "POST"
              && http_type  == mime_urlencoded)
        return "SRU POST";
    
    if ( http_method == "POST"
         && (http_type  == mime_text_xml
             || http_type  == mime_soap_xml))
        return "SRU SOAP";

    return "HTTP";
}

std::string 
yf::SRUtoZ3950::Rep::debug_http(const Z_HTTP_Request &http_req) const
{
    std::string message("<html>\n<body>\n<h1>"
                        "Metaproxy SRUtoZ3950 filter"
                        "</h1>\n");
    
    message += "<h3>HTTP Info</h3><br/>\n";
    message += "<p>\n";
    message += "<b>Method: </b> " + std::string(http_req.method) + "<br/>\n";
    message += "<b>Version:</b> " + std::string(http_req.version) + "<br/>\n";
    message += "<b>Path:   </b> " + std::string(http_req.path) + "<br/>\n";

    message += "<b>Content-Type:</b>"
        + http_header_value(http_req.headers, "Content-Type")
        + "<br/>\n";
    message += "<b>Content-Length:</b>"
        + http_header_value(http_req.headers, "Content-Length")
        + "<br/>\n";
    message += "</p>\n";    
    
    message += "<h3>Headers</h3><br/>\n";
    message += "<p>\n";    
    Z_HTTP_Header* header = http_req.headers;
    while (header){
        message += "<b>Header: </b> <i>" 
            + std::string(header->name) + ":</i> "
            + std::string(header->value) + "<br/>\n";
        header = header->next;
    }
    message += "</p>\n";    
    message += "</body>\n</html>\n";
    return message;
}

void yf::SRUtoZ3950::Rep::http_response(metaproxy_1::Package &package, 
                                        const std::string &content, 
                                        int http_code) const
{

    Z_GDU *zgdu_req = package.request().get(); 
    Z_GDU *zgdu_res = 0; 
    mp::odr odr;
    zgdu_res 
       = odr.create_HTTP_Response(package.session(), 
                                  zgdu_req->u.HTTP_Request, 
                                  http_code);
        
    zgdu_res->u.HTTP_Response->content_len = content.size();
    zgdu_res->u.HTTP_Response->content_buf 
        = (char*) odr_malloc(odr, zgdu_res->u.HTTP_Response->content_len);
    
    strncpy(zgdu_res->u.HTTP_Response->content_buf, 
            content.c_str(),  zgdu_res->u.HTTP_Response->content_len);
    
    //z_HTTP_header_add(odr, &hres->headers,
    //                  "Content-Type", content_type.c_str());
    package.response() = zgdu_res;
}




static mp::filter::Base* filter_creator()
{
    return new mp::filter::SRUtoZ3950;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_sru_to_z3950 = {
        0,
        "SRUtoZ3950",
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
