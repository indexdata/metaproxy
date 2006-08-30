/* $Id: gduutil.cpp,v 1.2 2006-08-30 08:35:47 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
*/

#include "gduutil.hpp"

#include <yaz/wrbuf.h>
#include <yaz/querytowrbuf.h>

#include <iostream>

namespace mp = metaproxy_1;

// Doxygen doesn't like mp::gdu, so we use this instead
namespace mp_gdu = metaproxy_1::gdu;

std::ostream& std::operator<<(std::ostream& os,  Z_GDU& zgdu)
{
    if (zgdu.which == Z_GDU_Z3950)
        os << "Z3950" << " " << *(zgdu.u.z3950) ;
    else if (zgdu.which == Z_GDU_HTTP_Request)
        os << "HTTP_Request" << " ";
    else if (zgdu.which == Z_GDU_HTTP_Response)
        os << "HTTP_Response" << " ";
    else
        os << "Z_GDU" << " ";
    return os;
}

std::ostream& std::operator<<(std::ostream& os,  Z_APDU& zapdu)
{
    switch(zapdu.which) {

    case Z_APDU_initRequest:
        os << "initRequest" << " ";
                        
        {
            Z_InitRequest *ir 
                = zapdu.u.initRequest;

            Z_IdAuthentication *a = ir->idAuthentication;
            if (a && a->which == Z_IdAuthentication_idPass )
                os << a->u.idPass->userId 
                   << ":" << a->u.idPass->groupId << " ";
            else
                os << "-:-" << " ";


            os << (ir->implementationId) << " "
                //<< ir->referenceId << " "
               << (ir->implementationName) << " "
               << (ir->implementationVersion) << " ";
        }
        break;
    case Z_APDU_initResponse:
        os << "initResponse" << " ";
        {
            Z_InitResponse *ir 
                = zapdu.u.initResponse;
            if (*(ir->result))
                os << "OK" << " "
                   << (ir->implementationId) << " "
                    //<< ir->referenceId << " "
                   << (ir->implementationName) << " "
                   << (ir->implementationVersion) << " ";
            else
                os << "DIAG" << " ";
        }
        break;
    case Z_APDU_searchRequest:
        os << "searchRequest" << " "
           << "--" << " ";
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
            os << wrbuf_buf(wr) << " ";
            wrbuf_free(wr, 1);
        }
        break;
    case Z_APDU_searchResponse:
        os << "searchResponse" << " ";
        {
            Z_SearchResponse *sr 
                = zapdu.u.searchResponse;
            if (*(sr->searchStatus))
                os << "OK" << " "
                   << *(sr->resultCount) << " "
                    //<< sr->referenceId << " "
                   << *(sr->numberOfRecordsReturned) << " "
                   << *(sr->nextResultSetPosition) << " ";
            else
                os << "DIAG" << " ";
        }
        break;
    case Z_APDU_presentRequest:
        os << "presentRequest" << " "
           << "--" << " "; 
        {
            Z_PresentRequest *pr = zapdu.u.presentRequest;
            os << pr->resultSetId << " "
                //<< pr->referenceId << " "
               << *(pr->resultSetStartPoint) << " "
               << *(pr->numberOfRecordsRequested) << " ";
        }
        break;
    case Z_APDU_presentResponse:
        os << "presentResponse" << " ";
        {
            Z_PresentResponse *pr 
                = zapdu.u.presentResponse;
            if (!*(pr->presentStatus))
                os << "OK" << " "
                   << "-" << " "
                    //<< pr->referenceId << " "
                   << *(pr->numberOfRecordsReturned) << " "
                   << *(pr->nextResultSetPosition) << " ";
            else
                os << "DIAG" << " "
                   << "-" << " "
                    //<< pr->referenceId << " "
                   << *(pr->numberOfRecordsReturned) << " "
                   << *(pr->nextResultSetPosition) << " ";
        }
        break;
    case Z_APDU_deleteResultSetRequest:
        os << "deleteResultSetRequest" << " ";
        break;
    case Z_APDU_deleteResultSetResponse:
        os << "deleteResultSetResponse" << " ";
        break;
    case Z_APDU_accessControlRequest:
        os << "accessControlRequest" << " ";
        break;
    case Z_APDU_accessControlResponse:
        os << "accessControlResponse" << " ";
        break;
    case Z_APDU_resourceControlRequest:
        os << "resourceControlRequest" << " ";
        break;
    case Z_APDU_resourceControlResponse:
        os << "resourceControlResponse" << " ";
        break;
    case Z_APDU_triggerResourceControlRequest:
        os << "triggerResourceControlRequest" << " ";
        break;
    case Z_APDU_resourceReportRequest:
        os << "resourceReportRequest" << " ";
        break;
    case Z_APDU_resourceReportResponse:
        os << "resourceReportResponse" << " ";
        break;
    case Z_APDU_scanRequest:
        os << "scanRequest" << " "
           << "--" << " ";
        { 
            Z_ScanRequest *sr 
                = zapdu.u.scanRequest;
                            
            for (int i = 0; i < sr->num_databaseNames; i++)
            {
                os << sr->databaseNames[i];
                if (i+1 ==  sr->num_databaseNames)
                    os << " ";
                else
                    os << "+";
            }

            os << *(sr->numberOfTermsRequested) << " "
               << *(sr->preferredPositionInResponse) << " "
               << *(sr->stepSize) << " ";
                         
            WRBUF wr = wrbuf_alloc();
            yaz_scan_to_wrbuf(wr, sr->termListAndStartPoint, VAL_NONE);
            os << wrbuf_buf(wr) << " ";
            wrbuf_free(wr, 1);
        }
        break;
    case Z_APDU_scanResponse:
        os << "scanResponse" << " ";
        {
            Z_ScanResponse *sr 
                = zapdu.u.scanResponse;
            if (*(sr->scanStatus))
                os << "OK" << " "
                   << *(sr->scanStatus) << " "
                   << *(sr->numberOfEntriesReturned) << " "
                    //<< sr->referenceId << " "
                   << *(sr->positionOfTerm) << " "
                   << *(sr->stepSize) << " ";
            else
                os << "DIAG" << " "
                   << *(sr->scanStatus) << " "
                   << *(sr->numberOfEntriesReturned) << " "
                    //<< sr->referenceId << " "
                   << *(sr->positionOfTerm) << " "
                   << *(sr->stepSize) << " ";
        }
        break;
    case Z_APDU_sortRequest:
        os << "sortRequest" << " ";
        break;
    case Z_APDU_sortResponse:
        os << "sortResponse" << " ";
        break;
    case Z_APDU_segmentRequest:
        os << "segmentRequest" << " ";
        break;
    case Z_APDU_extendedServicesRequest:
        os << "extendedServicesRequest" << " ";
//         { 
//             Z_ExtendedServicesRequest *er 
//                 = zapdu.u.extendedServicesRequest;

//             os << er->packageName << " "
//                << er->userId << " "
//                << er->description << " ";
//         }
        break;
    case Z_APDU_extendedServicesResponse:
        os << "extendedServicesResponse" << " ";
//         { 
//             Z_ExtendedServicesResponse *er 
//                 = zapdu.u.extendedServicesResponse;

//             os << *(er->operationStatus) << " "
//                << er->num_diagnostics << " ";
//         }
        break;
    case Z_APDU_close:
        os  << "close" << " ";
        { 
            Z_Close  *c 
                = zapdu.u.close;

            os << *(c->closeReason) << " ";
            switch (*(c->closeReason)) {
            case Z_Close_finished:
                os << "finished" << " ";
                break;
            case Z_Close_shutdown:
                os << "shutdown" << " ";
                break;
            case Z_Close_systemProblem:
                os << "systemProblem" << " ";
                break;
            case Z_Close_costLimit:
                os << "costLimit" << " ";
                break;
            case Z_Close_resources:
                os << "resources" << " ";
                break;
            case Z_Close_securityViolation:
                os << "securityViolation" << " ";
                break;
            case Z_Close_protocolError:
                os << "protocolError" << " ";
                break;
            case Z_Close_lackOfActivity:
                os << "" << " ";
                break;
            case Z_Close_peerAbort:
                os << "peerAbort" << " ";
                break;
            case Z_Close_unspecified:
                os << "unspecified" << " ";
                break;
            default:
                os << "unknown" << " ";
                break;
            }
        }
        break;
    case Z_APDU_duplicateDetectionRequest:
        os << "duplicateDetectionRequest" << " ";
        break;
    case Z_APDU_duplicateDetectionResponse:
        os << "duplicateDetectionResponse" << " ";
        break;
    default: 
        os << "Z_APDU "
           << "UNKNOWN" << " ";
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
