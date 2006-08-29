/* $Id: gduutil.cpp,v 1.1 2006-08-29 10:06:31 marc Exp $
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
        os << "initRequest" << " "
            << "OK" << " ";
                        
        {
            Z_InitRequest *ir 
                = zapdu.u.initRequest;
            os 
                << (ir->implementationId) << " "
                //<< ir->referenceId << " "
                << (ir->implementationName) << " "
                << (ir->implementationVersion) << " ";
        }
        break;
                    case Z_APDU_initResponse:
                        os<< "initResponse" << " ";
                        {
                            Z_InitResponse *ir 
                                = zapdu.u.initResponse;
                            if (*(ir->result))
                                os 
                                    << "OK" << " "
                                    << (ir->implementationId) << " "
                                    //<< ir->referenceId << " "
                                    << (ir->implementationName) << " "
                                    << (ir->implementationVersion) << " ";
                            else
                                os 
                                    << "ERROR" << " "
                                    << "- - -" << " ";

                         }
                        break;
    case Z_APDU_searchRequest:
        os << "searchRequest" << " "
            << "OK" << " ";
        { 
            Z_SearchRequest *sr 
                = zapdu.u.searchRequest;
                            
            for (int i = 0; i < sr->num_databaseNames; i++)
            {
                os << sr->databaseNames[i];
                if (i+1 ==  sr->num_databaseNames)
                    os<< " ";
                else
                    os<< "+";
            }
                         
            WRBUF wr = wrbuf_alloc();
            yaz_query_to_wrbuf(wr, sr->query);
            os << wrbuf_buf(wr) << " ";
            wrbuf_free(wr, 1);
        }
        break;
                    case Z_APDU_searchResponse:
                        os<< "searchResponse" << " ";
                        {
                            Z_SearchResponse *sr 
                                = zapdu.u.searchResponse;
                            if (*(sr->searchStatus))
                                os 
                                    << "OK" << " "
                                    << *(sr->resultCount) << " "
                                    //<< sr->referenceId << " "
                                    << *(sr->numberOfRecordsReturned) << " "
                                    << *(sr->nextResultSetPosition) << " ";
                            else
                                os 
                                    << "ERROR" << " "
                                    << "- - -" << " ";

                         }
                        //os<< wrbuf_buf(wr) << " ";
                        break;
    case Z_APDU_presentRequest:
        os << "presentRequest" << " "
            << "OK" << " "; 
        {
            Z_PresentRequest *pr = zapdu.u.presentRequest;
            os << pr->resultSetId << " "
                //<< pr->referenceId << " "
               << *(pr->resultSetStartPoint) << " "
               << *(pr->numberOfRecordsRequested) << " ";
        }
        break;
                   case Z_APDU_presentResponse:
                        os<< "presentResponse" << " ";
                        {
                            Z_PresentResponse *pr 
                                = zapdu.u.presentResponse;
                            if (!*(pr->presentStatus))
                                os 
                                    << "OK" << " "
                                    << "-" << " "
                                    //<< pr->referenceId << " "
                                    << *(pr->numberOfRecordsReturned) << " "
                                    << *(pr->nextResultSetPosition) << " ";
                            else
                                os 
                                    << "ERROR" << " "
                                    << "-" << " "
                                    //<< pr->referenceId << " "
                                    << *(pr->numberOfRecordsReturned) << " "
                                    << *(pr->nextResultSetPosition) << " ";
                                    //<< "- - -" << " ";
                         }
                        break;
    case Z_APDU_deleteResultSetRequest:
        os << "deleteResultSetRequest" << " "
           << "OK" << " ";
        break;
                     case Z_APDU_deleteResultSetResponse:
                        os << "deleteResultSetResponse" << " ";
                        break;
    case Z_APDU_accessControlRequest:
        os << "accessControlRequest" << " "
           << "OK" << " "; 
        break;
                     case Z_APDU_accessControlResponse:
                        os << "accessControlResponse" << " ";
                        break;
   case Z_APDU_resourceControlRequest:
        os << "resourceControlRequest" << " "
            << "OK" << " ";
        break;
                    case Z_APDU_resourceControlResponse:
                        os << "resourceControlResponse" << " ";
                        break;
    case Z_APDU_triggerResourceControlRequest:
        os << "triggerResourceControlRequest" << " "
            << "OK" << " ";
        break;
    case Z_APDU_resourceReportRequest:
        os << "resourceReportRequest" << " "
            << "OK" << " ";
        break;
                    case Z_APDU_resourceReportResponse:
                        os << "resourceReportResponse" << " ";
                        break;
     case Z_APDU_scanRequest:
        os << "scanRequest" << " "
            << "OK" << " ";
        break;
                    case Z_APDU_scanResponse:
                        os << "scanResponse" << " ";
                        break;
    case Z_APDU_sortRequest:
        os << "sortRequest" << " "
            << "OK" << " ";
        break;
                    case Z_APDU_sortResponse:
                        os << "sortResponse" << " ";
                        break;
    case Z_APDU_segmentRequest:
        os << "segmentRequest" << " "
            << "OK" << " ";
        break;
    case Z_APDU_extendedServicesRequest:
        os << "extendedServicesRequest" << " "
            << "OK" << " ";
        break;
                    case Z_APDU_extendedServicesResponse:
                        os << "extendedServicesResponse" << " ";
                        break;
    case Z_APDU_close:
        os  << "close" << " "
            << "OK" << " ";
        break;
    case Z_APDU_duplicateDetectionRequest:
        os << "duplicateDetectionRequest" << " "
           << "OK" << " ";
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
