/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

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

#include "sru_util.hpp"
#include <metaproxy/util.hpp>

#include <iostream>
#include <string>

namespace mp = metaproxy_1;

// Doxygen doesn't like mp::gdu, so we use this instead
namespace mp_util = metaproxy_1::util;

const std::string xmlns_explain("http://explain.z3950.org/dtd/2.0/");

bool mp_util::build_sru_debug_package(mp::Package &package)
{
    Z_GDU *zgdu_req = package.request().get();
    if (zgdu_req && zgdu_req->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request* http_req =  zgdu_req->u.HTTP_Request;
        std::string content = mp_util::http_headers_debug(*http_req);
        int http_code = 400;
        mp_util::http_response(package, content, http_code);
        return true;
    }
    package.session().close();
    return false;
}

mp_util::SRUServerInfo mp_util::get_sru_server_info(mp::Package &package)
{
    mp_util::SRUServerInfo sruinfo;

    // getting host and port info
    sruinfo.host = "localhost";
    sruinfo.port = "80";

    // overwriting host and port info if set from HTTP Host header
    Z_GDU *zgdu_req = package.request().get();
    if (zgdu_req && zgdu_req->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request* http_req =  zgdu_req->u.HTTP_Request;
        if (http_req)
        {
            std::string http_path = http_req->path;

            // taking out GET parameters
            std::string::size_type ipath = http_path.rfind("?");
            if (ipath != std::string::npos)
                http_path.assign(http_path, 0, ipath);

            // assign to database name
            if (http_path.size() > 1)
                sruinfo.database.assign(http_path, 1, std::string::npos);

            std::string http_host_address
                = mp_util::http_header_value(http_req->headers, "Host");

            std::string::size_type iaddress = http_host_address.rfind(":");
            if (iaddress != std::string::npos)
            {
                sruinfo.host.assign(http_host_address, 0, iaddress);
                sruinfo.port.assign(http_host_address, iaddress + 1,
                                    std::string::npos);
            }
        }
    }

    //std::cout << "sruinfo.database " << sruinfo.database << "\n";
    //std::cout << "sruinfo.host " << sruinfo.host << "\n";
    //std::cout << "sruinfo.port " << sruinfo.port << "\n";

    return sruinfo;
}


bool mp_util::build_sru_explain(metaproxy_1::Package &package,
                                metaproxy_1::odr &odr_en,
                                Z_SRW_PDU *sru_pdu_res,
                                SRUServerInfo sruinfo,
                                const xmlNode *explain,
                                Z_SRW_explainRequest const *er_req)
{

    // building SRU explain record
    std::string explain_xml;

    if (explain == 0)
    {
        explain_xml
            = mp_util::to_string(
                "<explain  xmlns=\"" + xmlns_explain + "\">\n"
                "  <serverInfo protocol='SRU'>\n"
                "    <host>")
            + sruinfo.host
            + mp_util::to_string("</host>\n"
                                 "    <port>")
            + sruinfo.port
            + mp_util::to_string("</port>\n"
                                 "    <database>")
            + sruinfo.database
            + mp_util::to_string("</database>\n"
                                 "  </serverInfo>\n"
                                 "</explain>\n");
    }
    else
    {
        // make new XML DOC with given explain node
        xmlDocPtr doc =  xmlNewDoc(BAD_CAST "1.0");
        xmlDocSetRootElement(doc, (xmlNode*)explain);

        xmlChar *xmlbuff;
        int xmlbuffsz;
        xmlDocDumpFormatMemory(doc, &xmlbuff, &xmlbuffsz, 1);

        explain_xml.assign((const char*)xmlbuff, 0, xmlbuffsz);
    }


    // z3950'fy recordPacking
    int record_packing = Z_SRW_recordPacking_XML;
    if (er_req && er_req->recordPacking && 's' == *(er_req->recordPacking))
        record_packing = Z_SRW_recordPacking_string;

    // preparing explain record insert
    Z_SRW_explainResponse *sru_res = sru_pdu_res->u.explain_response;

    // inserting one and only explain record

    sru_res->record.recordPosition = odr_intdup(odr_en, 1);
    sru_res->record.recordPacking = record_packing;
    sru_res->record.recordSchema = (char *)xmlns_explain.c_str();
    sru_res->record.recordData_len = 1 + explain_xml.size();
    sru_res->record.recordData_buf
        = odr_strdupn(odr_en, (const char *)explain_xml.c_str(),
                      1 + explain_xml.size());

    return true;
}


bool mp_util::build_sru_response(mp::Package &package,
                                 mp::odr &odr_en,
                                 Z_SOAP *soap,
                                 const Z_SRW_PDU *sru_pdu_res,
                                 char *charset,
                                 const char *stylesheet)
{

    // SRU request package translation to Z3950 package
    //if (sru_pdu_res)
    //    std::cout << *(const_cast<Z_SRW_PDU *>(sru_pdu_res)) << "\n";
    //else
    //    std::cout << "SRU empty\n";


    Z_GDU *zgdu_req = package.request().get();
    if  (zgdu_req && zgdu_req->which == Z_GDU_HTTP_Request)
    {
        Z_GDU *zgdu_res //= z_get_HTTP_Response(odr_en, 200);
            = odr_en.create_HTTP_Response(package.session(),
                                          zgdu_req->u.HTTP_Request,
                                          200);

        // adding HTTP response code and headers
        Z_HTTP_Response * http_res = zgdu_res->u.HTTP_Response;
        //http_res->code = http_code;

        std::string ctype("text/xml");
        if (charset)
        {
            ctype += "; charset=";
            ctype += charset;
        }

        z_HTTP_header_add(odr_en,
                          &http_res->headers, "Content-Type", ctype.c_str());

        // packaging Z_SOAP into HTML response
        static Z_SOAP_Handler soap_handlers[4] = {
            {(char *)YAZ_XMLNS_SRU_v1_1, 0, (Z_SOAP_fun) yaz_srw_codec},
            {(char *)YAZ_XMLNS_SRU_v1_0, 0,  (Z_SOAP_fun) yaz_srw_codec},
            {(char *)YAZ_XMLNS_UPDATE_v0_9, 0, (Z_SOAP_fun) yaz_ucp_codec},
            {0, 0, 0}
        };


        // empty stylesheet means NO stylesheet
        if (stylesheet && *stylesheet == '\0')
            stylesheet = 0;

        // encoding SRU package

        soap->u.generic->p  = (void*) sru_pdu_res;
        //int ret =
        z_soap_codec_enc_xsl(odr_en, &soap,
                             &http_res->content_buf, &http_res->content_len,
                             soap_handlers, charset, stylesheet);


        package.response() = zgdu_res;
        return true;
    }
    package.session().close();
    return false;
}



Z_SRW_PDU * mp_util::decode_sru_request(mp::Package &package,
                                        mp::odr &odr_de,
                                        mp::odr &odr_en,
                                        Z_SRW_PDU *sru_pdu_res,
                                        Z_SOAP **soap,
                                        char *charset,
                                        char *stylesheet)
{
    Z_GDU *zgdu_req = package.request().get();
    Z_SRW_PDU *sru_pdu_req = 0;

    //assert((zgdu_req->which == Z_GDU_HTTP_Request));

    //ignoring all non HTTP_Request packages
    if (!zgdu_req || !(zgdu_req->which == Z_GDU_HTTP_Request))
    {
        return 0;
    }

    Z_HTTP_Request* http_req =  zgdu_req->u.HTTP_Request;
    if (! http_req)
        return 0;

    // checking if we got a SRU GET/POST/SOAP HTTP package
    // closing connection if we did not ...
    if (0 == yaz_sru_decode(http_req, &sru_pdu_req, soap,
                            odr_de, &charset,
                            &(sru_pdu_res->u.response->diagnostics),
                            &(sru_pdu_res->u.response->num_diagnostics)))
    {
        if (sru_pdu_res->u.response->num_diagnostics)
        {
            //sru_pdu_res = sru_pdu_res_exp;
            package.session().close();
            return 0;
        }
        return sru_pdu_req;
    }
    else if (0 == yaz_srw_decode(http_req, &sru_pdu_req, soap,
                                 odr_de, &charset))
        return sru_pdu_req;
    else
    {
        //sru_pdu_res = sru_pdu_res_exp;
        package.session().close();
        return 0;
    }
    return 0;
}


bool
mp_util::check_sru_query_exists(mp::Package &package,
                                mp::odr &odr_en,
                                Z_SRW_PDU *sru_pdu_res,
                                Z_SRW_searchRetrieveRequest const *sr_req)
{
#ifdef Z_SRW_query_type_cql
    if ((sr_req->query_type == Z_SRW_query_type_cql && !sr_req->query.cql))
    {
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_MANDATORY_PARAMETER_NOT_SUPPLIED,
                               "query");
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_QUERY_SYNTAX_ERROR,
                               "CQL query is empty");
        return false;
    }
    if ((sr_req->query_type == Z_SRW_query_type_xcql && !sr_req->query.xcql))
    {
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_QUERY_SYNTAX_ERROR,
                               "XCQL query is empty");
        return false;
    }
    if ((sr_req->query_type == Z_SRW_query_type_pqf && !sr_req->query.pqf))
    {
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_QUERY_SYNTAX_ERROR,
                               "PQF query is empty");
        return false;
    }
#else
    if (!sr_req->query)
    {
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_MANDATORY_PARAMETER_NOT_SUPPLIED,
                               "query");
        yaz_add_srw_diagnostic(odr_en,
                               &(sru_pdu_res->u.response->diagnostics),
                               &(sru_pdu_res->u.response->num_diagnostics),
                               YAZ_SRW_QUERY_SYNTAX_ERROR,
                               "CQL query is empty");
        return false;
    }
#endif
    return true;
}


Z_ElementSetNames *
mp_util::build_esn_from_schema(mp::odr &odr_en, const char *schema)
{
    if (!schema)
        return 0;

    Z_ElementSetNames *esn
        = (Z_ElementSetNames *) odr_malloc(odr_en, sizeof(Z_ElementSetNames));
    esn->which = Z_ElementSetNames_generic;
    esn->u.generic = odr_strdup(odr_en, schema);
    return esn;
}


std::ostream& std::operator<<(std::ostream& os, Z_SRW_PDU& srw_pdu)
{
    os << "SRU";

    switch (srw_pdu.which)
    {
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

#ifdef Z_SRW_query_type_cql
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
#else
                os << " " << (sr->queryType ? sr->queryType : "cql")
                   << " " << sr->query;
#endif
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

