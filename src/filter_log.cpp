/* $Id: filter_log.cpp,v 1.35 2008-02-27 11:08:49 adam Exp $
   Copyright (c) 2005-2008, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include "filter_log.hpp"
#include "config.hpp"
#include "package.hpp"

#include <string>
#include <sstream>
#include <iomanip>
#include <boost/thread/mutex.hpp>

#include "gduutil.hpp"
#include "util.hpp"
#include "xmlutil.hpp"

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

void yf::Log::configure(const xmlNode *xmlnode, bool test_only)
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
                if (!strcmp((const char *) attr->name,  "access"))
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
    Z_GDU *gdu = package.request().get();
    std::string user("-");

    yaz_timing_t timer = yaz_timing_create();

    // scope for session lock
    {
        boost::mutex::scoped_lock scoped_lock(m_session_mutex);
        
        if (gdu && gdu->which == Z_GDU_Z3950)
        {
            Z_APDU *apdu_req = gdu->u.z3950;
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
            if (gdu)          
            {
                std::ostringstream os;
                os  << m_msg_config << " "
                    << package << " "
                    << "0.000000" << " " 
                    << *gdu;
                m_file->log(m_time_format, os);
            }
        }

        if (m_user_access)
        {
            if (gdu)          
            {
                std::ostringstream os;
                os  << m_msg_config << " " << user << " "
                    << package << " "
                    << "0.000000" << " " 
                    << *gdu;
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
            if (gdu && gdu->which == Z_GDU_Z3950 &&
                gdu->u.z3950->which == Z_APDU_initRequest)
            {
                std::ostringstream os;
                os << m_msg_config << " init options:";
                yaz_init_opt_decode(gdu->u.z3950->u.initRequest->options,
                                    option_write, &os);
                m_file->log(m_time_format, os);
            }
        }
        
        if (m_req_apdu)
        {
            if (gdu)
            {
                mp::odr odr(ODR_PRINT);
                odr_set_stream(odr, m_file->fhandle, stream_write, 0);
                z_GDU(odr, &gdu, 0, 0);
            }
        }
    }
    
    // unlocked during move
    package.move();

    gdu = package.response().get();

    yaz_timing_stop(timer);
    double duration = yaz_timing_get_real(timer);

    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_file->m_mutex);

        if (m_access)
        {
            if (gdu)
            {
                std::ostringstream os;
                os  << m_msg_config << " "
                    << package << " "
                    << std::fixed << std::setprecision (6) << duration
                    << " "
                    << *gdu;
                m_file->log(m_time_format, os);
            }
        }
        if (m_user_access)
        {
            if (gdu)
            {
                std::ostringstream os;
                os  << m_msg_config << " " << user << " "
                    << package << " "
                    << std::fixed << std::setprecision (6) << duration << " "
                    << *gdu;
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
            if (gdu && gdu->which == Z_GDU_Z3950 &&
                gdu->u.z3950->which == Z_APDU_initResponse)
            {
                std::ostringstream os;
                os << m_msg_config;
                os << " init options:";
                yaz_init_opt_decode(gdu->u.z3950->u.initResponse->options,
                                    option_write, &os);
                m_file->log(m_time_format, os);
            }
        }
        
        if (m_res_apdu)
        {
            if (gdu)
            {
                mp::odr odr(ODR_PRINT);
                odr_set_stream(odr, m_file->fhandle, stream_write, 0);
                z_GDU(odr, &gdu, 0, 0);
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
