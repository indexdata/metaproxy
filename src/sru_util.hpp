/* $Id: sru_util.hpp,v 1.2 2006-10-02 13:44:48 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_SDU_UTIL_HPP
#define YP2_SDU_UTIL_HPP

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

        class SRU 
        {
        public:
            enum SRU_protocol_type { SRU_NONE, SRU_GET, SRU_POST, SRU_SOAP};
            typedef const int& SRU_query_type;
            union SRW_query {char * cql; char * xcql; char * pqf;};
        private:
            //bool decode(const Z_HTTP_Request &http_req);
            SRU_protocol_type protocol(const Z_HTTP_Request &http_req) const;
        private:
            SRU_protocol_type m_protocol;
            std::string m_charset;
            std::string m_stylesheet;            
        };

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
