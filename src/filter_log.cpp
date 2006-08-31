/* $Id: filter_log.cpp,v 1.26 2006-08-31 12:56:40 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "filter_log.hpp"
#include "config.hpp"
#include "package.hpp"

#include <string>
#include <sstream>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "gduutil.hpp"
#include "util.hpp"
#include "xmlutil.hpp"

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
    m_access = true;
    m_req_apdu = false;
    m_res_apdu = false;
    m_req_session = false;
    m_res_session = false;
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



    // scope for locking Ostream 
    { 
        boost::mutex::scoped_lock scoped_lock(m_p->m_file->m_mutex);
        
 
        if (m_p->m_access)
        {
            gdu = package.request().get();
            if (gdu)          
            {
                m_p->m_file->out
                    //<< receive_time << " "
                    //<< to_iso_string(receive_time) << " "
                    << to_iso_extended_string(receive_time) << " "
                    << m_p->m_msg_config << " "
                    << package << " "
                    << "000000.000000" << " " 
                    << *gdu
                    << "\n";
            }
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
            if (gdu)
            {
                m_p->m_file->out
                    //<< send_time << " "
                    //<< to_iso_string(send_time) << " "
                    << to_iso_extended_string(send_time) << " "
                    << m_p->m_msg_config << " "
                    << package << " "
                    << to_iso_string(duration) << " "
                    << *gdu
                    << "\n";
            }   
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
