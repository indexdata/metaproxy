/* $Id: filter_log.cpp,v 1.22 2006-08-28 21:40:24 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "package.hpp"

#include <string>
#include <sstream>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "util.hpp"
#include "xmlutil.hpp"
#include "filter_log.hpp"

#include <fstream>
#include <yaz/zgdu.h>
#include <yaz/wrbuf.h>
#include <yaz/querytowrbuf.h>


namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class Log::LFile {
        public:
            boost::mutex m_mutex;
            std::string m_fname;
            std::ofstream fout;
            std::ostream &out;
            LFile(std::string fname);
            LFile(std::string fname, std::ostream &use_this);
        };
        typedef boost::shared_ptr<Log::LFile> LFilePtr;
        class Log::Rep {
            friend class Log;
            Rep();
            void openfile(const std::string &fname);
            std::string m_msg_config;
            LFilePtr m_file;
            bool m_access;
            bool m_req_apdu;
            bool m_res_apdu;
            bool m_req_session;
            bool m_res_session;
            bool m_init_options;
        };
        // Only used during configure stage (no threading)
        static std::list<LFilePtr> filter_log_files;
    }
}

yf::Log::Rep::Rep()
{
    m_req_apdu = true;
    m_res_apdu = true;
    m_req_session = true;
    m_res_session = true;
    m_init_options = false;
    openfile("");
}

yf::Log::Log(const std::string &x) : m_p(new Rep)
{
    m_p->m_msg_config = x;
}

yf::Log::Log() : m_p(new Rep)
{
}

yf::Log::~Log() {}

void stream_write(ODR o, void *handle, int type, const char *buf, int len)
{
    yf::Log::LFile *lfile = (yf::Log::LFile*) handle;
    lfile->out.write(buf, len);
}

void option_write(const char *name, void *handle)
{
    yf::Log::LFile *lfile = (yf::Log::LFile*) handle;
    lfile->out << " " << name;
}

void yf::Log::process(mp::Package &package) const
{
    Z_GDU *gdu;

    // getting timestamp for receiving of package
    boost::posix_time::ptime receive_time
        = boost::posix_time::microsec_clock::local_time();


    std::ostringstream msg_request;
    std::ostringstream msg_request_2;
    std::ostringstream msg_response;
    std::ostringstream msg_response_2;

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_p->m_file->m_mutex);
        
 
        if (m_p->m_access)
        {
            
            gdu = package.request().get();
            WRBUF wr = wrbuf_alloc();

           if (gdu && gdu->which == Z_GDU_Z3950)          
                {
                  
                    msg_request << "Z39.50" << " ";

                    switch(gdu->u.z3950->which)
                    {
                    case Z_APDU_initRequest:
                        msg_request 
                            << "initRequest" << " "
                            << "OK" << " ";
                        
                        {
                            Z_InitRequest *ir 
                                = gdu->u.z3950->u.initRequest;
                            msg_request_2 
                                << (ir->implementationId) << " "
                                //<< ir->referenceId << " "
                                << (ir->implementationName) << " "
                                << (ir->implementationVersion) << " ";
                         }
                        break;
                    case Z_APDU_searchRequest:
                        msg_request 
                            << "searchRequest" << " "
                            << "OK" << " ";
                        {
                            Z_SearchRequest *sr 
                                = gdu->u.z3950->u.searchRequest;
                            
                            for (int i = 0; i < sr->num_databaseNames; i++)
                            {
                                msg_request << sr->databaseNames[i];
                                if (i+1 ==  sr->num_databaseNames)
                                    msg_request << " ";
                                else
                                    msg_request << "+";
                            }
                         
                            yaz_query_to_wrbuf(wr, sr->query);
                        }
                        msg_request_2 << wrbuf_buf(wr) << " ";
                        break;
                    case Z_APDU_presentRequest:
                        msg_request 
                            << "presentRequest" << " "
                            << "OK" << " "; 
                        {
                            Z_PresentRequest *pr 
                                = gdu->u.z3950->u.presentRequest;
                                msg_request_2 
                                    << pr->resultSetId << " "
                                    //<< pr->referenceId << " "
                                    << *(pr->resultSetStartPoint) << " "
                                    << *(pr->numberOfRecordsRequested) << " ";
                        }
                        break;
                    case Z_APDU_deleteResultSetRequest:
                        msg_request 
                            << "deleteResultSetRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_accessControlRequest:
                        msg_request 
                            << "accessControlRequest" << " "
                            << "OK" << " "; 
                        break;
                    case Z_APDU_resourceControlRequest:
                        msg_request 
                            << "resourceControlRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_triggerResourceControlRequest:
                        msg_request 
                            << "triggerResourceControlRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_resourceReportRequest:
                        msg_request 
                            << "resourceReportRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_scanRequest:
                        msg_request 
                            << "scanRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_sortRequest:
                        msg_request 
                            << "sortRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_segmentRequest:
                        msg_request 
                            << "segmentRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_extendedServicesRequest:
                        msg_request 
                            << "extendedServicesRequest" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_close:
                        msg_response 
                            << "close" << " "
                            << "OK" << " ";
                        break;
                    case Z_APDU_duplicateDetectionRequest:
                        msg_request 
                            << "duplicateDetectionRequest" << " "
                            << "OK" << " ";
                        break;
                    default: 
                        msg_request 
                            << "unknown" << " "
                            << "ERROR" << " ";
                    }
                }
           else if (gdu && gdu->which == Z_GDU_HTTP_Request)
               msg_request << "HTTP " << "unknown " ;
           else if (gdu && gdu->which == Z_GDU_HTTP_Response)
               msg_request << "HTTP-Response " << "unknown " ;
           else
               msg_request << "unknown " << "unknown " ;

           wrbuf_free(wr, 1);

           m_p->m_file->out
               << m_p->m_msg_config << " "
               << package.session().id() << " "
               << receive_time << " "
               // << send_time << " "
               << "00:00:00.000000" << " " 
               // << duration  << " "
               << msg_request.str()
               << msg_request_2.str()
               //<< msg_response.str()
               //<< msg_response_2.str()
               << "\n";
        }

        if (m_p->m_req_session)
        {
            m_p->m_file->out << receive_time << " " << m_p->m_msg_config;
            m_p->m_file->out << " request id=" << package.session().id();
            m_p->m_file->out << " close=" 
                             << (package.session().is_closed() ? "yes" : "no")
                             << "\n";
        }
        if (m_p->m_init_options)
        {
            gdu = package.request().get();
            if (gdu && gdu->which == Z_GDU_Z3950 &&
                gdu->u.z3950->which == Z_APDU_initRequest)
            {
                m_p->m_file->out << receive_time << " " << m_p->m_msg_config;
                m_p->m_file->out << " init options:";
                yaz_init_opt_decode(gdu->u.z3950->u.initRequest->options,
                                    option_write, m_p->m_file.get());
                m_p->m_file->out << "\n";
            }
        }

        if (m_p->m_req_apdu)
        {
            gdu = package.request().get();
            if (gdu)
            {
                mp::odr odr(ODR_PRINT);
                odr_set_stream(odr, m_p->m_file.get(), stream_write, 0);
                z_GDU(odr, &gdu, 0, 0);
            }
        }
        m_p->m_file->out.flush();
    }

    // unlocked during move
    package.move();

    // getting timestamp for sending of package
    boost::posix_time::ptime send_time
        = boost::posix_time::microsec_clock::local_time();

    boost::posix_time::time_duration duration = send_time - receive_time;

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_p->m_file->m_mutex);
        if (m_p->m_access)
        {
            gdu = package.response().get();
            //WRBUF wr = wrbuf_alloc();


           if (gdu && gdu->which == Z_GDU_Z3950)          
                {
                  
                    msg_response << "Z39.50" << " ";

                    switch(gdu->u.z3950->which)
                    {
                    case Z_APDU_initResponse:
                        msg_response << "initResponse" << " ";
                        {
                            Z_InitResponse *ir 
                                = gdu->u.z3950->u.initResponse;
                            if (*(ir->result))
                                msg_response_2 
                                    << "OK" << " "
                                    << (ir->implementationId) << " "
                                    //<< ir->referenceId << " "
                                    << (ir->implementationName) << " "
                                    << (ir->implementationVersion) << " ";
                            else
                                msg_response_2 
                                    << "ERROR" << " "
                                    << "- - -" << " ";

                         }
                        break;
                    case Z_APDU_searchResponse:
                        msg_response << "searchResponse" << " ";
                        {
                            Z_SearchResponse *sr 
                                = gdu->u.z3950->u.searchResponse;
                            if (*(sr->searchStatus))
                                msg_response_2 
                                    << "OK" << " "
                                    << *(sr->resultCount) << " "
                                    //<< sr->referenceId << " "
                                    << *(sr->numberOfRecordsReturned) << " "
                                    << *(sr->nextResultSetPosition) << " ";
                            else
                                msg_response_2 
                                    << "ERROR" << " "
                                    << "- - -" << " ";

                         }
                        //msg_response << wrbuf_buf(wr) << " ";
                        break;
                    case Z_APDU_presentResponse:
                        msg_response << "presentResponse" << " ";
                        {
                            Z_PresentResponse *pr 
                                = gdu->u.z3950->u.presentResponse;
                            if (!*(pr->presentStatus))
                                msg_response_2 
                                    << "OK" << " "
                                    << "-" << " "
                                    //<< pr->referenceId << " "
                                    << *(pr->numberOfRecordsReturned) << " "
                                    << *(pr->nextResultSetPosition) << " ";
                            else
                                msg_response_2 
                                    << "ERROR" << " "
                                    << "-" << " "
                                    //<< pr->referenceId << " "
                                    << *(pr->numberOfRecordsReturned) << " "
                                    << *(pr->nextResultSetPosition) << " ";
                                    //<< "- - -" << " ";
                         }
                        break;
                    case Z_APDU_deleteResultSetResponse:
                        msg_response << "deleteResultSetResponse" << " ";
                        break;
                    case Z_APDU_accessControlResponse:
                        msg_response << "accessControlResponse" << " ";
                        break;
                    case Z_APDU_resourceControlResponse:
                        msg_response << "resourceControlResponse" << " ";
                        break;
                        //case Z_APDU_triggerResourceControlResponse:
                        //msg_response << "triggerResourceControlResponse" << " ";
                        //break;
                    case Z_APDU_resourceReportResponse:
                        msg_response << "resourceReportResponse" << " ";
                        break;
                    case Z_APDU_scanResponse:
                        msg_response << "scanResponse" << " ";
                        break;
                    case Z_APDU_sortResponse:
                        msg_response << "sortResponse" << " ";
                        break;
                        // case Z_APDU_segmentResponse:
                        // msg_response << "segmentResponse" << " ";
                        // break;
                    case Z_APDU_extendedServicesResponse:
                        msg_response << "extendedServicesResponse" << " ";
                        break;
                    case Z_APDU_close:
                        msg_response << "close" << " ";
                        break;
                    case Z_APDU_duplicateDetectionResponse:
                        msg_response << "duplicateDetectionResponse" << " ";
                        break;
                    default: 
                        msg_response << "unknown" << " ";
                    }
                }
           else if (gdu && gdu->which == Z_GDU_HTTP_Request)
               msg_response << "HTTP " << "unknown " ;
           else if (gdu && gdu->which == Z_GDU_HTTP_Response)
               msg_response << "HTTP-Response " << "unknown " ;
           else
               msg_response << "unknown " << "unknown " ;

           m_p->m_file->out
               << m_p->m_msg_config << " "
               << package.session().id() << " "
               // << receive_time << " "
                << send_time << " "
               //<< "-" << " "
                << duration  << " "
               //<< msg_request.str()
               //<< msg_request_2.str()
               << msg_response.str()
               << msg_response_2.str()
               << "\n";

            //wrbuf_free(wr, 1);
        }
        if (m_p->m_res_session)
        {
            m_p->m_file->out << send_time << " " << m_p->m_msg_config;
            m_p->m_file->out << " response id=" << package.session().id();
            m_p->m_file->out << " close=" 
                             << (package.session().is_closed() ? "yes " : "no ")
                             << "duration=" << duration      
                             << "\n";
        }
        if (m_p->m_init_options)
        {
            gdu = package.response().get();
            if (gdu && gdu->which == Z_GDU_Z3950 &&
                gdu->u.z3950->which == Z_APDU_initResponse)
            {
                m_p->m_file->out << receive_time << " " << m_p->m_msg_config;
                m_p->m_file->out << " init options:";
                yaz_init_opt_decode(gdu->u.z3950->u.initResponse->options,
                                    option_write, m_p->m_file.get());
                m_p->m_file->out << "\n";
            }
        }
        if (m_p->m_res_apdu)
        {
            gdu = package.response().get();
            if (gdu)
            {
                mp::odr odr(ODR_PRINT);
                odr_set_stream(odr, m_p->m_file.get(), stream_write, 0);
                z_GDU(odr, &gdu, 0, 0);
            }
        }
        m_p->m_file->out.flush();
    }
}

yf::Log::LFile::LFile(std::string fname) : 
    m_fname(fname), fout(fname.c_str()), out(fout)
{
}

yf::Log::LFile::LFile(std::string fname, std::ostream &use_this) : 
    m_fname(fname), out(use_this)
{
}

void yf::Log::Rep::openfile(const std::string &fname)
{
    std::list<LFilePtr>::const_iterator it = filter_log_files.begin();
    for (; it != filter_log_files.end(); it++)
    {
        if ((*it)->m_fname == fname)
        {
            m_file = *it;
            return;
        }
    }
    // open stdout for empty file
    LFilePtr newfile(fname.length() == 0 
                     ? new LFile(fname, std::cout) 
                     : new LFile(fname));
    filter_log_files.push_back(newfile);
    m_file = newfile;
}

void yf::Log::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "message"))
            m_p->m_msg_config = mp::xml::get_text(ptr);
        else if (!strcmp((const char *) ptr->name, "filename"))
        {
            std::string fname = mp::xml::get_text(ptr);
            m_p->openfile(fname);
        }
        else if (!strcmp((const char *) ptr->name, "category"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, 
                                 "access"))
                    m_p->m_access = 
                        mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "request-apdu"))
                    m_p->m_req_apdu = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "response-apdu"))
                    m_p->m_res_apdu = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "apdu"))
                {
                    m_p->m_req_apdu = mp::xml::get_bool(attr->children, true);
                    m_p->m_res_apdu = m_p->m_req_apdu;
                }
                else if (!strcmp((const char *) attr->name,
                                 "request-session"))
                    m_p->m_req_session = 
                        mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, 
                                 "response-session"))
                    m_p->m_res_session = 
                        mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name,
                                 "session"))
                {
                    m_p->m_req_session = 
                        mp::xml::get_bool(attr->children, true);
                    m_p->m_res_session = m_p->m_req_session;
                }
                else if (!strcmp((const char *) attr->name, 
                                 "init-options"))
                    m_p->m_init_options = 
                        mp::xml::get_bool(attr->children, true);
                else
                    throw mp::filter::FilterException(
                        "Bad attribute " + std::string((const char *)
                                                       attr->name));
            }
        }
        else
        {
            throw mp::filter::FilterException("Bad element " 
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::Log;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_log = {
        0,
        "log",
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
