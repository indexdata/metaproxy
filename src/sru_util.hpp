/* $Id: sru_util.hpp,v 1.8 2007-03-20 07:20:16 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_SDU_UTIL_HPP
#define YP2_SDU_UTIL_HPP

#include "util.hpp"
#include "package.hpp"

//#include <yaz/zgdu.h>
//#include <yaz/z-core.h>
#include <yaz/srw.h>

#include <iosfwd>
#include <string>

namespace std 
{
    std::ostream& operator<<(std::ostream& os, Z_SRW_PDU& srw_pdu); 

}


namespace metaproxy_1 {
    namespace util  {

        class SRUServerInfo;

        // std::string sru_protocol(const Z_HTTP_Request &http_req);
        // std::string debug_http(const Z_HTTP_Request &http_req);
        // void http_response(mp::Package &package, 
        //                   const std::string &content, 
        //                   int http_code = 200);

        bool build_sru_debug_package(metaproxy_1::Package &package);

        SRUServerInfo get_sru_server_info(metaproxy_1::Package &package);
                                          // Z_SRW_explainRequest 
                                          //const *er_req);
        
//         bool build_simple_explain(metaproxy_1::Package &package, 
//                                   metaproxy_1::odr &odr_en,
//                                   Z_SRW_PDU *sru_pdu_res,
//                                   SRUServerInfo sruinfo,
//                                   Z_SRW_explainRequest const *er_req = 0);
        
        bool build_sru_explain(metaproxy_1::Package &package, 
                               metaproxy_1::odr &odr_en,
                               Z_SRW_PDU *sru_pdu_res,
                               SRUServerInfo sruinfo,
                               const xmlNode *explain = 0,
                               Z_SRW_explainRequest const *er_req = 0);
        
        bool build_sru_response(metaproxy_1::Package &package, 
                                metaproxy_1::odr &odr_en,
                                Z_SOAP *soap,
                                const Z_SRW_PDU *sru_pdu_res,
                                char *charset,
                                const char *stylesheet);        
        
        Z_SRW_PDU * decode_sru_request(metaproxy_1::Package &package,   
                                       metaproxy_1::odr &odr_de,
                                       metaproxy_1::odr &odr_en,
                                       Z_SRW_PDU *sru_pdu_res,
                                       Z_SOAP **soap,
                                       char *charset,
                                       char *stylesheet);

        bool check_sru_query_exists(metaproxy_1::Package &package,
                                    metaproxy_1::odr &odr_en,
                                    Z_SRW_PDU *sru_pdu_res,
                                    Z_SRW_searchRetrieveRequest
                                    const *sr_req);
        
        Z_ElementSetNames * build_esn_from_schema(metaproxy_1::odr &odr_en, 
                                                  const char *schema);

        class SRUServerInfo
        {
        public:
            SRUServerInfo ()
                : database("Default")
                {}
        public:
            std::string database;
            std::string host;
            std::string port;
        };
        
        
        

//         class SRU 
//         {
//         public:
//             enum SRU_protocol_type { SRU_NONE, SRU_GET, SRU_POST, SRU_SOAP};
//             typedef const int SRU_query_type;
//             union SRW_query {char * cql; char * xcql; char * pqf;};
//         private:
//             //bool decode(const Z_HTTP_Request &http_req);
//             SRU_protocol_type protocol(const Z_HTTP_Request &http_req) const;
//         private:
//             SRU_protocol_type m_protocol;
//             std::string m_charset;
//             std::string m_stylesheet;            
//         };
    }    
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
