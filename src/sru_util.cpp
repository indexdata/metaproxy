/* $Id: sru_util.cpp,v 1.2 2006-10-02 13:44:48 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
*/

#include "sru_util.hpp"
#include "util.hpp"

//#include <yaz/wrbuf.h>
//#include <yaz/querytowrbuf.h>

#include <iostream>
#include <string>

namespace mp = metaproxy_1;

// Doxygen doesn't like mp::gdu, so we use this instead
namespace mp_util = metaproxy_1::util;


mp_util::SRU::SRU_protocol_type
mp_util::SRU::protocol(const Z_HTTP_Request &http_req) const
{
    const std::string mime_urlencoded("application/x-www-form-urlencoded");
    const std::string mime_text_xml("text/xml");
    const std::string mime_soap_xml("application/soap+xml");

    const std::string http_method(http_req.method);
    const std::string http_type 
        =  mp_util::http_header_value(http_req.headers, "Content-Type");

    if (http_method == "GET")
        return SRU_GET;

    if (http_method == "POST"
              && http_type  == mime_urlencoded)
        return SRU_POST;
    
    if ( http_method == "POST"
         && (http_type  == mime_text_xml
             || http_type  == mime_soap_xml))
        return SRU_SOAP;

    return SRU_NONE;
}


std::ostream& std::operator<<(std::ostream& os, Z_SRW_PDU& srw_pdu) 
{
    os << "SRU";
    
    switch(srw_pdu.which) {
    case  Z_SRW_searchRetrieve_request:
        os << " " << "searchRetrieveRequest";
        {
            Z_SRW_searchRetrieveRequest *sr = srw_pdu.u.request;
            if (sr)
            {
                if (sr->database)
                    os << " " << (sr->database);
                else
                    os << " -";
                if (sr->startRecord)
                    os << " " << *(sr->startRecord);
                else
                    os << " -";
                if (sr->maximumRecords)
                    os << " " << *(sr->maximumRecords);
                else
                    os << " -";
                if (sr->recordPacking)
                    os << " " << (sr->recordPacking);
                else
                    os << " -";

                if (sr->recordSchema)
                    os << " " << (sr->recordSchema);
                else
                    os << " -";
                
                switch (sr->query_type){
                case Z_SRW_query_type_cql:
                    os << " CQL";
                    if (sr->query.cql)
                        os << " " << sr->query.cql;
                    break;
                case Z_SRW_query_type_xcql:
                    os << " XCQL";
                    break;
                case Z_SRW_query_type_pqf:
                    os << " PQF";
                    if (sr->query.pqf)
                        os << " " << sr->query.pqf;
                    break;
                }
            }
        }
        break;
    case  Z_SRW_searchRetrieve_response:
        os << " " << "searchRetrieveResponse";
        {
            Z_SRW_searchRetrieveResponse *sr = srw_pdu.u.response;
            if (sr)
            {
                if (! (sr->num_diagnostics))
                {
                    os << " OK";
                    if (sr->numberOfRecords)
                        os << " " << *(sr->numberOfRecords);
                    else
                        os << " -";
                    //if (sr->num_records)
                    os << " " << (sr->num_records);
                    //else
                    //os << " -";
                    if (sr->nextRecordPosition)
                        os << " " << *(sr->nextRecordPosition);
                    else
                        os << " -";
                } 
                else
                {
                    os << " DIAG";
                    if (sr->diagnostics && sr->diagnostics->uri)
                        os << " " << (sr->diagnostics->uri);
                    else
                        os << " -";
                    if (sr->diagnostics && sr->diagnostics->message)
                        os << " " << (sr->diagnostics->message);
                    else
                        os << " -";
                    if (sr->diagnostics && sr->diagnostics->details)
                        os << " " << (sr->diagnostics->details);
                    else
                        os << " -";
                }
                
                    
            }
        }
        break;
    case  Z_SRW_explain_request:
        os << " " << "explainRequest";
        break;
    case  Z_SRW_explain_response:
        os << " " << "explainResponse";
        break;
    case  Z_SRW_scan_request:
        os << " " << "scanRequest";
        break;
    case  Z_SRW_scan_response:
        os << " " << "scanResponse";
        break;
    case  Z_SRW_update_request:
        os << " " << "updateRequest";
        break;
    case  Z_SRW_update_response:
        os << " " << "updateResponse";
        break;
    default: 
        os << " " << "UNKNOWN";    
    }

    return os;    
}



/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
