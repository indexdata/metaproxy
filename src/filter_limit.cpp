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

#include "config.hpp"
#include "filter_limit.hpp"

#include <time.h>
#include <yaz/log.h>
#include <yazpp/timestat.h>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#ifdef WIN32
#include <windows.h>
#endif

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Limit::Ses {
        public:
            yazpp_1::TimeStat bw_stat;
            yazpp_1::TimeStat pdu_stat;
            yazpp_1::TimeStat search_stat;
            Ses() : bw_stat(60), pdu_stat(60), search_stat(60) {};
        };

        class Limit::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
        private:
            boost::mutex m_session_mutex;
            std::map<mp::Session,Limit::Ses *> m_sessions;
            int m_bw_max;
            int m_pdu_max;
            int m_search_max;
            int m_max_record_retrieve;
        };
    }
}

// define Pimpl wrapper forwarding to Impl

yf::Limit::Limit() : m_p(new Impl)
{
}

yf::Limit::~Limit()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Limit::configure(const xmlNode *xmlnode, bool test_only,
                          const char *path)
{
    m_p->configure(xmlnode);
}

void yf::Limit::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff

yf::Limit::Impl::Impl() : m_bw_max(0), m_pdu_max(0), m_search_max(0),
                          m_max_record_retrieve(0)
{
}

yf::Limit::Impl::~Impl()
{
}

void yf::Limit::Impl::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "limit"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "bandwidth"))
                    m_bw_max = mp::xml::get_int(attr->children, 0);
                else if (!strcmp((const char *) attr->name, "pdu"))
                    m_pdu_max = mp::xml::get_int(attr->children, 0);
                else if (!strcmp((const char *) attr->name, "search"))
                    m_search_max = mp::xml::get_int(attr->children, 0);
                else if (!strcmp((const char *) attr->name, "retrieve"))
                    m_max_record_retrieve =
                        mp::xml::get_int(attr->children, 0);
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

void yf::Limit::Impl::process(mp::Package &package)
{
    int sz = 0;
    {
        boost::mutex::scoped_lock scoped_lock(m_session_mutex);

        yf::Limit::Ses *ses = 0;

        std::map<mp::Session,yf::Limit::Ses *>::iterator it =
            m_sessions.find(package.session());
        if (it != m_sessions.end())
            ses = it->second;
        else
        {
            ses = new yf::Limit::Ses;
            m_sessions[package.session()] = ses;
        }


        Z_GDU *gdu = package.request().get();
        if (gdu && gdu->which == Z_GDU_Z3950)
        {
            sz += package.request().get_size();
            // we're getting a Z39.50 package
            Z_APDU *apdu = gdu->u.z3950;
            if (apdu->which == Z_APDU_searchRequest)
                ses->search_stat.add_bytes(1);
            if (m_max_record_retrieve)
            {
                if (apdu->which == Z_APDU_presentRequest)
                {
                    Z_PresentRequest *pr = apdu->u.presentRequest;
                    if (pr->numberOfRecordsRequested &&
                        *pr->numberOfRecordsRequested > m_max_record_retrieve)
                        *pr->numberOfRecordsRequested = m_max_record_retrieve;
                }
            }
        }
    }
    package.move();
    int reduce = 0;
    {
        boost::mutex::scoped_lock scoped_lock(m_session_mutex);

        yf::Limit::Ses *ses = 0;

        std::map<mp::Session,yf::Limit::Ses *>::iterator it =
            m_sessions.find(package.session());
        if (it != m_sessions.end())
            ses = it->second;
        else
        {
            ses = new yf::Limit::Ses;
            m_sessions[package.session()] = ses;
        }

        sz += package.response().get_size();

        ses->bw_stat.add_bytes(sz);
        ses->pdu_stat.add_bytes(1);

        int bw_total = ses->bw_stat.get_total();
        int pdu_total = ses->pdu_stat.get_total();
        int search_total = ses->search_stat.get_total();

        if (m_search_max)
            reduce += search_total / m_search_max;
        if (m_bw_max)
            reduce += (bw_total/m_bw_max);
        if (m_pdu_max)
        {
            if (pdu_total > m_pdu_max)
            {
                int nreduce = (m_pdu_max >= 60) ? 1 : 60/m_pdu_max;
                reduce = (reduce > nreduce) ? reduce : nreduce;
            }
        }
        if (package.session().is_closed())
        {
            m_sessions.erase(package.session());
            delete ses;
        }
    }
    if (reduce)
    {
        yaz_log(YLOG_LOG, "sleeping %d seconds", reduce);
#ifdef WIN32
        Sleep(reduce * 1000);
#else
        sleep(reduce);
#endif
    }
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::Limit;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_limit = {
        0,
        "limit",
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

