/* $Id: gduutil.cpp,v 1.16 2006-10-05 12:17:24 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
*/

#include "gduutil.hpp"
#include "util.hpp"

#include <yaz/wrbuf.h>
#include <yaz/querytowrbuf.h>

#include <iostream>
#include <list>

namespace mp = metaproxy_1;

// Doxygen doesn't like mp::gdu, so we use this instead
namespace mp_util = metaproxy_1::util;




std::ostream& std::operator<<(std::ostream& os,  Z_GDU& zgdu)
{
    if (zgdu.which == Z_GDU_Z3950)
    {
        os << "Z3950";
        if (zgdu.u.z3950)
            os << *(zgdu.u.z3950);
    }
    else if (zgdu.which == Z_GDU_HTTP_Request)
    {
        os << "HTTP_Request";
        if (zgdu.u.HTTP_Request)
            os << " " << *(zgdu.u.HTTP_Request);
    }
    else if (zgdu.which == Z_GDU_HTTP_Response)
    {
        os << "HTTP_Response";
        if (zgdu.u.HTTP_Response)
            os << " " << *(zgdu.u.HTTP_Response);
    }
    else
        os << "Z_GDU";
    return os;
}

std::ostream& std::operator<<(std::ostream& os, Z_HTTP_Request& httpreq)
{
    os << httpreq.method << " ";
    os << httpreq.path;    
    return os;
}


std::ostream& std::operator<<(std::ostream& os, Z_HTTP_Response& httpres)
{
    os << httpres.code << " ";
    os << httpres.content_len;   
    return os;
}

std::ostream& std::operator<<(std::ostream& os, Z_Records & rs)
{
    switch(rs.which) {
    case Z_Records_DBOSD :
        break;
    case Z_Records_NSD:
        if (rs.u.nonSurrogateDiagnostic)
            os << *(rs.u.nonSurrogateDiagnostic);
        break;
    case Z_Records_multipleNSD:
        os << "Z_Records_multipleNSD";
        //os << *(rs.u.multipleNonSurDiagnostics);
        break;
    default:
        os << "Z_Records" ;
    }
    
    return os;
}

std::ostream& std::operator<<(std::ostream& os, Z_DiagRec& dr)
{
    switch(dr.which) {
    case Z_DiagRec_defaultFormat:
        if (dr.u.defaultFormat)
            os << *(dr.u.defaultFormat);
        break;
    case Z_DiagRec_externallyDefined :
        os << "Z_DiagRec_externallyDefined";
        break;
    default:
        os << "Z_DiagRec" ;
    }
    
    return os;
}

std::ostream& std::operator<<(std::ostream& os, Z_DefaultDiagFormat& ddf)
{
    if (ddf.condition)
        os << *(ddf.condition) << " ";

    switch(ddf.which) {
    case Z_DefaultDiagFormat_v2Addinfo:
        os << ddf.u.v2Addinfo;
        break;
    case Z_DefaultDiagFormat_v3Addinfo:
        os << ddf.u.v3Addinfo;
        break;
    default:
        os << "Z_DefaultDiagFormat" ;
    }
    
    return os;
}

std::ostream& std::operator<<(std::ostream& os,  Z_APDU& zapdu)
{
    switch(zapdu.which) {

    case Z_APDU_initRequest:
        os << " " << "initRequest" << " ";
                        
        {
            Z_InitRequest *ir 
                = zapdu.u.initRequest;

            Z_IdAuthentication *a = ir->idAuthentication;
            if (a && a->which == Z_IdAuthentication_idPass )
                os << a->u.idPass->userId << " ";
            //<< ":" << a->u.idPass->groupId << " ";
            else
                os << "-" << " ";

            std::list<std::string> vhosts;
            mp::util::get_vhost_otherinfo(ir->otherInfo, vhosts);
            if (vhosts.size()){
                copy(vhosts.begin(), vhosts.end(), 
                     ostream_iterator<string>(os, " "));
            }
                else
                    os << "-" << " " ;

            os << (ir->implementationId) << " "
                //<< ir->referenceId << " "
               << (ir->implementationName) << " "
               << (ir->implementationVersion);
        }
        break;
    case Z_APDU_initResponse:
        os << " " << "initResponse" << " ";
        {
            Z_InitResponse *ir 
                = zapdu.u.initResponse;
            if (ir->result && *(ir->result))
                os << "OK" << " "
                   << (ir->implementationId) << " "
                    //<< ir->referenceId << " "
                   << (ir->implementationName) << " "
                   << (ir->implementationVersion) << " ";
            else
                os << "DIAG";
        }
        break;
    case Z_APDU_searchRequest:
        os << " " << "searchRequest" << " ";
        { 
            Z_SearchRequest *sr 
                = zapdu.u.searchRequest;
                            
            for (int i = 0; i < sr->num_databaseNames; i++)
            {
                os << sr->databaseNames[i];
                if (i+1 ==  sr->num_databaseNames)
                    os << " ";
                else
                    os << "+";
            }
                         
            WRBUF wr = wrbuf_alloc();
            yaz_query_to_wrbuf(wr, sr->query);
            os << wrbuf_buf(wr);
            wrbuf_free(wr, 1);
        }
        break;
    case Z_APDU_searchResponse:
        os << " " << "searchResponse" << " ";
        {
            Z_SearchResponse *sr 
                = zapdu.u.searchResponse;
            if (sr->searchStatus && *(sr->searchStatus))
            {
                os << "OK";
                if (sr->resultCount)
                    os << " " << *(sr->resultCount);
                else
                    os << " -";
                //<< sr->referenceId << " "
                if (sr->numberOfRecordsReturned)
                    os << " " << *(sr->numberOfRecordsReturned);
                else
                    os << " -";
                if (sr->nextResultSetPosition)
                    os << " " << *(sr->nextResultSetPosition);
                else
                    os << " -";
            }
            else 
                if (sr->records)
                    os << "DIAG " << *(sr->records);
                else
                    os << "ERROR";
        }
        break;
    case Z_APDU_presentRequest:
        os << " " << "presentRequest";
        {
            Z_PresentRequest *pr = zapdu.u.presentRequest;
            if (pr->resultSetId)
                os << " " << (pr->resultSetId);
            else
                os << " -";
            //<< pr->referenceId << " "
            if (pr->resultSetStartPoint)
                os << " " << *(pr->resultSetStartPoint);
            else
                os << " -";
            if (pr->numberOfRecordsRequested)
                os << " " << *(pr->numberOfRecordsRequested);
            else
                os << " -";
            if (pr->preferredRecordSyntax)
                //os << " " << pr->preferredRecordSyntax;
                os << " " <<(oid_getentbyoid(pr->preferredRecordSyntax))->desc;
            else
                os << " -";
            const char * msg = 0;
            if (pr->recordComposition 
                && (msg = mp_util::record_composition_to_esn(pr->recordComposition)))
                os << " " << msg;
            else
                os << " -";
        }
        break;
    case Z_APDU_presentResponse:
        os << " " << "presentResponse" << " ";
        {
            Z_PresentResponse *pr 
                = zapdu.u.presentResponse;
            if ((pr->presentStatus) && !*(pr->presentStatus))
            {
                os << "OK";
                //<< pr->referenceId << " "
                if (pr->numberOfRecordsReturned)
                    os << " " << *(pr->numberOfRecordsReturned);
                else
                    os << " -";
                if (pr->nextResultSetPosition)
                    os << " " << *(pr->nextResultSetPosition);
                else
                    os << " -";
            }
            else
                if (pr->records)
                    os << "DIAG " << *(pr->records);
                else
                    os << "ERROR";

            //os << "DIAG" << " "
            //<< "-" << " "
            //<< pr->referenceId << " "
            //<< *(pr->numberOfRecordsReturned) << " "
            //<< *(pr->nextResultSetPosition);
        }
        break;
    case Z_APDU_deleteResultSetRequest:
        os << " " << "deleteResultSetRequest";
        break;
    case Z_APDU_deleteResultSetResponse:
        os << " " << "deleteResultSetResponse";
        break;
    case Z_APDU_accessControlRequest:
        os << " " << "accessControlRequest";
        break;
    case Z_APDU_accessControlResponse:
        os << " " << "accessControlResponse";
        break;
    case Z_APDU_resourceControlRequest:
        os << " " << "resourceControlRequest";
        break;
    case Z_APDU_resourceControlResponse:
        os << " " << "resourceControlResponse";
        break;
    case Z_APDU_triggerResourceControlRequest:
        os << " " << "triggerResourceControlRequest";
        break;
    case Z_APDU_resourceReportRequest:
        os << " " << "resourceReportRequest";
        break;
    case Z_APDU_resourceReportResponse:
        os << " " << "resourceReportResponse";
        break;
    case Z_APDU_scanRequest:
        os << " " << "scanRequest" << " ";
        { 
            Z_ScanRequest *sr 
                = zapdu.u.scanRequest;
                        
            if (sr)
            {
                for (int i = 0; i < sr->num_databaseNames; i++)
                {
                    os << sr->databaseNames[i];
                    if (i+1 ==  sr->num_databaseNames)
                        os << " ";
                    else
                    os << "+";
                }
                if (sr->numberOfTermsRequested)
                    os << " " << *(sr->numberOfTermsRequested);
                else
                     os << " -";
                if (sr->preferredPositionInResponse)
                    os << " " << *(sr->preferredPositionInResponse);
                else
                    os << " -";
                if (sr->stepSize)
                    os << " " << *(sr->stepSize);
                else
                    os << " -";
                
                if (sr->termListAndStartPoint)
                {
                    WRBUF wr = wrbuf_alloc();
                    yaz_scan_to_wrbuf(wr, sr->termListAndStartPoint, VAL_NONE);
                    os << wrbuf_buf(wr);
                    wrbuf_free(wr, 1);
                }
                else
                    os << " -";
            }
        }
        break;
    case Z_APDU_scanResponse:
        os << " " << "scanResponse" << " ";
        {
            Z_ScanResponse *sr 
                = zapdu.u.scanResponse;
            if (sr)
            {
                if ((sr->scanStatus) && !*(sr->scanStatus))
                {
                    os << "OK";
                    //<< *(sr->scanStatus) << " "
                    if (sr->numberOfEntriesReturned)
                        os << " " << *(sr->numberOfEntriesReturned);
                    else
                        os << " -";
                    //<< sr->referenceId << " "
                    if (sr->positionOfTerm)
                        os << " " << *(sr->positionOfTerm);
                    else
                        os << " -";
                    if (sr->stepSize)
                        os << " " << *(sr->stepSize);
                     else
                        os << " -";                  
                }
                else {
                    os << "ERROR";
                    if (sr->scanStatus)
                    {
                        os << " " << *(sr->scanStatus) << " ";
                    
                        switch (*(sr->scanStatus)){
                        case Z_Scan_success:
                            os << "success ";
                            break;
                        case Z_Scan_partial_1:
                            os << "partial_1";
                            break;
                        case Z_Scan_partial_2:
                            os << "partial_2";
                            break;
                        case Z_Scan_partial_3:
                            os << "partial_3";
                            break;
                        case Z_Scan_partial_4:
                            os << "partial_4";
                            break;
                        case Z_Scan_partial_5:
                            os << "partial_5";
                            break;
                        case Z_Scan_failure:
                            os << "failure";
                            break;
                        default:
                            os << "unknown";
                        }
                    }
                    if (sr->numberOfEntriesReturned)
                        os << " " << *(sr->numberOfEntriesReturned);
                    else
                        os << " -";
                }
            }
        }
        break;
    case Z_APDU_sortRequest:
        os << " " << "sortRequest" << " ";
        break;
    case Z_APDU_sortResponse:
        os << " " << "sortResponse" << " ";
        break;
    case Z_APDU_segmentRequest:
        os << " " << "segmentRequest" << " ";
        break;
    case Z_APDU_extendedServicesRequest:
        os << " " << "extendedServicesRequest";
        { 
            Z_ExtendedServicesRequest *er 
                = zapdu.u.extendedServicesRequest;
            if (er)
            {
                if (er->function)
                {
                    os << " ";
                    switch(*(er->function))
                    {
                    case Z_ExtendedServicesRequest_create:
                        os << "create";
                        break;
                    case Z_ExtendedServicesRequest_delete:
                        os << "delete";
                        break;
                    case Z_ExtendedServicesRequest_modify:
                        os << "modify";
                        break;
                    default:
                        os << "unknown";
                    }
                }
                else
                    os << " -";
                    
                
                if (er->userId)
                    os << " " << er->userId ;
                else
                    os << " -";
                
                if (er->packageName)
                    os << " " << er->packageName;
                else
                    os << " -";
                
                if (er->description)
                    os << " " << er->description;
                else
                    os << " -";
            }
        }
        break;
    case Z_APDU_extendedServicesResponse:
        os << " " << "extendedServicesResponse";
         { 
             Z_ExtendedServicesResponse *er 
                 = zapdu.u.extendedServicesResponse;
             if (er)
             {
                 if (er->operationStatus)
                 {
                     os << " ";
                     switch (*(er->operationStatus)){
                     case Z_ExtendedServicesResponse_done:
                         os << "OK";
                         break;
                     case Z_ExtendedServicesResponse_accepted:
                         os << "ACCEPT";
                         break;
                     case Z_ExtendedServicesResponse_failure:
                         if (er->num_diagnostics)
                             os << "DIAG " << **(er->diagnostics);
                         else
                             os << "ERROR";
                         break;
                     default:
                         os << "unknown";
                     }
                 }
                 else
                     os << " -";
             }
         }
        break;
    case Z_APDU_close:
        os  << " " << "close" << " ";
        { 
            Z_Close  *c 
                = zapdu.u.close;
            if (c)
            {
                if (c->closeReason)
                {
                    os << *(c->closeReason) << " ";

                    switch (*(c->closeReason)) {
                    case Z_Close_finished:
                        os << "finished";
                        break;
                    case Z_Close_shutdown:
                        os << "shutdown";
                        break;
                    case Z_Close_systemProblem:
                        os << "systemProblem";
                        break;
                    case Z_Close_costLimit:
                        os << "costLimit";
                        break;
                    case Z_Close_resources:
                        os << "resources";
                        break;
                    case Z_Close_securityViolation:
                        os << "securityViolation";
                        break;
                    case Z_Close_protocolError:
                        os << "protocolError";
                        break;
                    case Z_Close_lackOfActivity:
                        os << "lackOfActivity";
                        break;
                    case Z_Close_peerAbort:
                        os << "peerAbort";
                        break;
                    case Z_Close_unspecified:
                        os << "unspecified";
                        break;
                    default:
                        os << "unknown";
                    }
                }
                
                if (c->diagnosticInformation)
                    os << " " << c->diagnosticInformation;
            }
        }
        break;
    case Z_APDU_duplicateDetectionRequest:
        os << " " << "duplicateDetectionRequest";
        break;
    case Z_APDU_duplicateDetectionResponse:
        os << " " << "duplicateDetectionResponse";
        break;
    default: 
        os << " " << "Z_APDU " << "UNKNOWN";
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
