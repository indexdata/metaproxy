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

#include "config.hpp"
#include "filter_log.hpp"
#include <metaproxy/package.hpp>

#include <string>
#include <sstream>
#include <iomanip>
#include <boost/thread/mutex.hpp>

#include "gduutil.hpp"
#include <metaproxy/util.hpp>
#include <metaproxy/xmlutil.hpp>

#include <yaz/zgdu.h>
#include <yaz/wrbuf.h>
#include <yaz/log.h>
#include <yaz/querytowrbuf.h>
#include <yaz/timing.h>
#include <stdio.h>
#include <time.h>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class Log::Impl {
        public:
            class LFile;
            typedef boost::shared_ptr<Log::Impl::LFile> LFilePtr;
        public:
            //Impl();
            Impl(const std::string &x = "-");
           ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
        private:
            void openfile(const std::string &fname);
            // needs to be static to be called by C pointer-to-function-syntax
            static void stream_write(ODR o, void *handle, int type, 
                              const char *buf, int len);
            // needs to be static to be called by C pointer-to-function-syntax
            static void option_write(const char *name, void *handle);
        private:
            std::string m_msg_config;
            bool m_1line;
            bool m_access;
            bool m_user_access;
            bool m_req_apdu;
            bool m_res_apdu;
            bool m_req_session;
            bool m_res_session;
            bool m_init_options;
            LFilePtr m_file;
            std::string m_time_format;
            // Only used during confiqgure stage (no threading), 
            // for performance avoid opening files which other log filter 
            // instances already have opened
            static std::list<LFilePtr> filter_log_files;

            boost::mutex m_session_mutex;
            std::map<mp::Session, std::string> m_sessions;
       };

        class Log::Impl::LFile {
        public:
            boost::mutex m_mutex;
            std::string m_fname;
            FILE *fhandle;
            ~LFile();
            LFile(std::string fname);
            LFile(std::string fname, FILE *outf);
            void log(const std::string &date_format,
                     std::ostringstream &os);
            void flush();
        };
        
    }
}

// define Pimpl wrapper forwarding to Impl
 
yf::Log::Log() : m_p(new Impl)
{
}

yf::Log::Log(const std::string &x) : m_p(new Impl(x))
{
}

yf::Log::~Log()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Log::configure(const xmlNode *xmlnode, bool test_only,
                        const char *path)
{
    m_p->configure(xmlnode);
}

void yf::Log::process(mp::Package &package) const
{
    m_p->process(package);
}


// static initialization
std::list<yf::Log::Impl::LFilePtr> yf::Log::Impl::filter_log_files;


yf::Log::Impl::Impl(const std::string &x)
    : m_msg_config(x),
      m_1line(false),
      m_access(true),
      m_user_access(false),
      m_req_apdu(false),
      m_res_apdu(false),
      m_req_session(false),
      m_res_session(false),
      m_init_options(false),
      m_time_format("%H:%M:%S-%d/%m")
{
    openfile("");
}


yf::Log::Impl::~Impl() 
{
}

static void log_DefaultDiagFormat(WRBUF w, Z_DefaultDiagFormat *e)
{
    if (e->condition)
        wrbuf_printf(w, ODR_INT_PRINTF " ",*e->condition);
    else
        wrbuf_puts(w, "?? ");
    if (e->which == Z_DefaultDiagFormat_v2Addinfo && e->u.v2Addinfo)
        wrbuf_puts(w, e->u.v2Addinfo);
    else if (e->which == Z_DefaultDiagFormat_v3Addinfo && e->u.v3Addinfo)
        wrbuf_puts(w, e->u.v3Addinfo);
}

static void log_DiagRecs(WRBUF w, int num_diagRecs, Z_DiagRec **diags)
{
    if (diags[0]->which != Z_DiagRec_defaultFormat)
        wrbuf_puts(w ,"(diag not in default format?)");
    else
    {
        Z_DefaultDiagFormat *e = diags[0]->u.defaultFormat;
        log_DefaultDiagFormat(w, e);
    }
}

static void log_1_line(Z_APDU *z_req, Z_APDU *z_res, WRBUF w)
{
    switch (z_req->which)
    {
    case Z_APDU_initRequest:
        if (z_res->which == Z_APDU_initResponse)
        {
            Z_InitRequest *req = z_req->u.initRequest;
            Z_InitResponse *res = z_res->u.initResponse;
            wrbuf_printf(w, "Init ");
            if (res->result && *res->result)
                wrbuf_printf(w, "OK -");
            else
            {
                Z_External *uif = res->userInformationField;
                bool got_code = false;
                wrbuf_printf(w, "ERROR ");
                if (uif && uif->which == Z_External_userInfo1)
                {
                    Z_OtherInformation *ui = uif->u.userInfo1;
                    if (ui->num_elements >= 1)
                    {
                        Z_OtherInformationUnit *unit = ui->list[0];
                        if (unit->which == Z_OtherInfo_externallyDefinedInfo &&
                            unit->information.externallyDefinedInfo &&
                            unit->information.externallyDefinedInfo->which ==
                            Z_External_diag1)
                        {
                            Z_DiagnosticFormat *diag =
                                unit->information.externallyDefinedInfo->
                                u.diag1;
                            if (diag->num >= 1)
                            {
                                Z_DiagnosticFormat_s *ds = diag->elements[0];
                                if (ds->which ==
                                    Z_DiagnosticFormat_s_defaultDiagRec)
                                {
                                    log_DefaultDiagFormat(w,
                                                          ds->u.defaultDiagRec);
                                    got_code = true;
                                }
                            }

                        }
                    }
                }
                if (!got_code)
                    wrbuf_puts(w, "-");
            }
            wrbuf_printf(w, " ID:%s Name:%s Version:%s",
                         req->implementationId ? req->implementationId :"-", 
                         req->implementationName ?req->implementationName : "-",
                         req->implementationVersion ?
                         req->implementationVersion : "-");
        }
        break;
    case Z_APDU_searchRequest:
        if (z_res->which == Z_APDU_searchResponse)
        {
            Z_SearchRequest *req = z_req->u.searchRequest;
            Z_SearchResponse *res = z_res->u.searchResponse;
            int i;
            wrbuf_puts(w, "Search ");
            for (i = 0 ; i < req->num_databaseNames; i++)
            {
                if (i)
                    wrbuf_printf(w, "+");
                wrbuf_puts(w, req->databaseNames[i]);
            }
            wrbuf_printf(w, " ");
            if (!res->records)
            {
                wrbuf_printf(w, "OK " ODR_INT_PRINTF " %s", *res->resultCount,
                             req->resultSetName);
            }
            else if (res->records->which == Z_Records_DBOSD)
            {
                wrbuf_printf(w, "OK " ODR_INT_PRINTF " %s", *res->resultCount,
                             req->resultSetName);
            }
            else if (res->records->which == Z_Records_NSD)
            {
                wrbuf_puts(w, "ERROR ");
                log_DefaultDiagFormat(w,
                                      res->records->u.nonSurrogateDiagnostic);
            }
            else if (res->records->which == Z_Records_multipleNSD)
            {
                wrbuf_puts(w, "ERROR ");
                log_DiagRecs(
                    w, 
                    res->records->u.multipleNonSurDiagnostics->num_diagRecs,
                    res->records->u.multipleNonSurDiagnostics->diagRecs);
            }
            wrbuf_printf(w, " 1+" ODR_INT_PRINTF " ",
                         res->numberOfRecordsReturned
                         ? *res->numberOfRecordsReturned : 0);
            yaz_query_to_wrbuf(w, req->query);
        }
        break;
    case Z_APDU_presentRequest:
        if (z_res->which == Z_APDU_presentResponse)
        {
            Z_PresentRequest *req = z_req->u.presentRequest;
            Z_PresentResponse *res = z_res->u.presentResponse;

            wrbuf_printf(w, "Present ");

            if (!res->records)
            {
                wrbuf_printf(w, "OK");
            }
            else if (res->records->which == Z_Records_DBOSD)
            {
                wrbuf_printf(w, "OK");
            }
            else if (res->records->which == Z_Records_NSD)
            {
                wrbuf_puts(w, "ERROR ");
                log_DefaultDiagFormat(w,
                                      res->records->u.nonSurrogateDiagnostic);
            }
            else if (res->records->which == Z_Records_multipleNSD)
            {
                wrbuf_puts(w, "ERROR ");
                log_DiagRecs(
                    w, 
                    res->records->u.multipleNonSurDiagnostics->num_diagRecs,
                    res->records->u.multipleNonSurDiagnostics->diagRecs);
            }
            wrbuf_printf(w, " %s " ODR_INT_PRINTF "+" ODR_INT_PRINTF " ",
                req->resultSetId, *req->resultSetStartPoint,
                         *req->numberOfRecordsRequested);
        }
        break;
    case Z_APDU_scanRequest:
        if (z_res->which == Z_APDU_scanResponse)
        {
            Z_ScanRequest *req = z_req->u.scanRequest;
            Z_ScanResponse *res = z_res->u.scanResponse;
            int i;
            wrbuf_printf(w, "Scan ");
            for (i = 0 ; i < req->num_databaseNames; i++)
            {
                if (i)
                    wrbuf_printf(w, "+");
                wrbuf_puts(w, req->databaseNames[i]);
            }
            wrbuf_puts(w, " ");
            if (!res->scanStatus || *res->scanStatus == 0)
                wrbuf_puts(w, "OK");
            else if (*res->scanStatus == 6)
                wrbuf_puts(w, "FAIL");
            else
                wrbuf_printf(w, "PARTIAL" ODR_INT_PRINTF, *res->scanStatus);
            
            wrbuf_printf(w, " " ODR_INT_PRINTF " " ODR_INT_PRINTF "+" 
                         ODR_INT_PRINTF "+" ODR_INT_PRINTF " ",
                         res->numberOfEntriesReturned ?
                         *res->numberOfEntriesReturned : 0,
                         req->preferredPositionInResponse ?
                          *req->preferredPositionInResponse : 1,
                         *req->numberOfTermsRequested,
                         res->stepSize ? *res->stepSize : 1);
            
            yaz_scan_to_wrbuf(w, req->termListAndStartPoint, 
                              req->attributeSet);
        }
        break;
    default:
        wrbuf_printf(w, "REQ=%d RES=%d", z_req->which, z_res->which);
    }
}

void yf::Log::Impl::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "message"))
            m_msg_config = mp::xml::get_text(ptr);
        else if (!strcmp((const char *) ptr->name, "filename"))
        {
            std::string fname = mp::xml::get_text(ptr);
            openfile(fname);
        }
        else if (!strcmp((const char *) ptr->name, "time-format"))
        {
            m_time_format = mp::xml::get_text(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "category"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name,  "line"))
                    m_1line = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name,  "access"))
                    m_access = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "user-access"))
                    m_user_access = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "request-apdu"))
                    m_req_apdu = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "response-apdu"))
                    m_res_apdu = mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, "apdu"))
                {
                    m_req_apdu = mp::xml::get_bool(attr->children, true);
                    m_res_apdu = m_req_apdu;
                }
                else if (!strcmp((const char *) attr->name,
                                 "request-session"))
                    m_req_session = 
                        mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, 
                                 "response-session"))
                    m_res_session = 
                        mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name,
                                 "session"))
                {
                    m_req_session = 
                        mp::xml::get_bool(attr->children, true);
                    m_res_session = m_req_session;
                }
                else if (!strcmp((const char *) attr->name, 
                                 "init-options"))
                    m_init_options = 
                        mp::xml::get_bool(attr->children, true);
                else if (!strcmp((const char *) attr->name, 
                                 "init-options"))
                    m_init_options = 
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

void yf::Log::Impl::process(mp::Package &package)
{
    Z_GDU *gdu_req = package.request().get();
    std::string user("-");

    yaz_timing_t timer = yaz_timing_create();

    // scope for session lock
    {
        boost::mutex::scoped_lock scoped_lock(m_session_mutex);
        
        if (gdu_req && gdu_req->which == Z_GDU_Z3950)
        {
            Z_APDU *apdu_req = gdu_req->u.z3950;
            if (apdu_req->which == Z_APDU_initRequest)
            {
                Z_InitRequest *req = apdu_req->u.initRequest;
                Z_IdAuthentication *a = req->idAuthentication;
                if (a)
                {
                    if (a->which == Z_IdAuthentication_idPass)
                        user = a->u.idPass->userId;
                    else if (a->which == Z_IdAuthentication_open)
                        user = a->u.open;
                
                    m_sessions[package.session()] = user;
                }
            }
        }
        std::map<mp::Session,std::string>::iterator it = 
            m_sessions.find(package.session());
        if (it != m_sessions.end())
            user = it->second;
        
        if (package.session().is_closed())
            m_sessions.erase(package.session());
    }
    // scope for locking Ostream
    { 
        boost::mutex::scoped_lock scoped_lock(m_file->m_mutex);
 
        if (m_access)
        {
            if (gdu_req)          
            {
                std::ostringstream os;
                os  << m_msg_config << " "
                    << package << " "
                    << "0.000000" << " " 
                    << *gdu_req;
                m_file->log(m_time_format, os);
            }
        }

        if (m_user_access)
        {
            if (gdu_req)          
            {
                std::ostringstream os;
                os  << m_msg_config << " " << user << " "
                    << package << " "
                    << "0.000000" << " " 
                    << *gdu_req;
                m_file->log(m_time_format, os);
            }
        }

        if (m_req_session)
        {
            std::ostringstream os;
            os << m_msg_config;
            os << " request id=" << package.session().id();
            os << " close=" 
               << (package.session().is_closed() ? "yes" : "no");
            m_file->log(m_time_format, os);
        }

        if (m_init_options)
        {
            if (gdu_req && gdu_req->which == Z_GDU_Z3950 &&
                gdu_req->u.z3950->which == Z_APDU_initRequest)
            {
                std::ostringstream os;
                os << m_msg_config << " init options:";
                yaz_init_opt_decode(gdu_req->u.z3950->u.initRequest->options,
                                    option_write, &os);
                m_file->log(m_time_format, os);
            }
        }
        
        if (m_req_apdu)
        {
            if (gdu_req)
            {
                mp::odr odr(ODR_PRINT);
                odr_set_stream(odr, m_file->fhandle, stream_write, 0);
                z_GDU(odr, &gdu_req, 0, 0);
            }
        }
    }
    
    // unlocked during move
    package.move();

    Z_GDU *gdu_res = package.response().get();

    yaz_timing_stop(timer);
    double duration = yaz_timing_get_real(timer);

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_file->m_mutex);
        
        if (m_1line)
        {
            if (gdu_req && gdu_res && gdu_req->which == Z_GDU_Z3950
                && gdu_res->which == Z_GDU_Z3950)
            {
                mp::wrbuf w;

                log_1_line(gdu_req->u.z3950, gdu_res->u.z3950, w);
                const char *message = wrbuf_cstr(w);

                std::ostringstream os;
                os  << m_msg_config << " "
                    << package << " "
                    << std::fixed << std::setprecision (6) << duration
                    << " "
                    << message;
                m_file->log(m_time_format, os);
            }
        }

        if (m_access)
        {
            if (gdu_res)
            {
                std::ostringstream os;
                os  << m_msg_config << " "
                    << package << " "
                    << std::fixed << std::setprecision (6) << duration
                    << " "
                    << *gdu_res;
                m_file->log(m_time_format, os);
            }
        }
        if (m_user_access)
        {
            if (gdu_res)
            {
                std::ostringstream os;
                os  << m_msg_config << " " << user << " "
                    << package << " "
                    << std::fixed << std::setprecision (6) << duration << " "
                    << *gdu_res;
                m_file->log(m_time_format, os);
            }   
        }

        if (m_res_session)
        {
            std::ostringstream os;
            os << m_msg_config;
            os << " response id=" << package.session().id();
            os << " close=" 
               << (package.session().is_closed() ? "yes " : "no ")
               << "duration=" 
               << std::fixed << std::setprecision (6) << duration;
            m_file->log(m_time_format, os);
        }

        if (m_init_options)
        {
            if (gdu_res && gdu_res->which == Z_GDU_Z3950 &&
                gdu_res->u.z3950->which == Z_APDU_initResponse)
            {
                std::ostringstream os;
                os << m_msg_config;
                os << " init options:";
                yaz_init_opt_decode(gdu_res->u.z3950->u.initResponse->options,
                                    option_write, &os);
                m_file->log(m_time_format, os);
            }
        }
        
        if (m_res_apdu)
        {
            if (gdu_res)
            {
                mp::odr odr(ODR_PRINT);
                odr_set_stream(odr, m_file->fhandle, stream_write, 0);
                z_GDU(odr, &gdu_res, 0, 0);
            }
        }
    }
    m_file->flush();
    yaz_timing_destroy(&timer);
}


void yf::Log::Impl::openfile(const std::string &fname)
{
    std::list<LFilePtr>::const_iterator it
        = filter_log_files.begin();
    for (; it != filter_log_files.end(); it++)
    {
        if ((*it)->m_fname == fname)
        {
            m_file = *it;
            return;
        }
    }
    LFilePtr newfile(new LFile(fname));
    filter_log_files.push_back(newfile);
    m_file = newfile;
}


void yf::Log::Impl::stream_write(ODR o, void *handle, int type, const char *buf, int len)
{
    FILE *f = (FILE*) handle;
    fwrite(buf, len, 1, f ? f : yaz_log_file());
}

void yf::Log::Impl::option_write(const char *name, void *handle)
{
    std::ostringstream *os = (std::ostringstream *) handle;
    *os << " " << name;
}


yf::Log::Impl::LFile::LFile(std::string fname) : 
    m_fname(fname)
    
{
    if (fname.c_str())
        fhandle = fopen(fname.c_str(), "a");
    else
        fhandle = 0;
}

yf::Log::Impl::LFile::~LFile()
{
}

void yf::Log::Impl::LFile::log(const std::string &date_format,
                               std::ostringstream &os)
{
    if (fhandle)
    {
        char datestr[80];
        time_t ti = time(0);
#if HAVE_LOCALTIME_R
        struct tm tm0, *tm = &tm0;
        localtime_r(&ti, tm);
#else
        struct tm *tm = localtime(&ti);
#endif
        if (strftime(datestr, sizeof(datestr)-1, date_format.c_str(), tm))
        {
            fputs(datestr, fhandle);
            fputs(" ", fhandle);
        }
        fputs(os.str().c_str(), fhandle);
        fputc('\n', fhandle);
    }    
    else
        yaz_log(YLOG_LOG, "%s", os.str().c_str());
}

void yf::Log::Impl::LFile::flush()
{
    if (fhandle)
        fflush(fhandle);
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

