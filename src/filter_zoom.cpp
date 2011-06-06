/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

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
#include "filter_zoom.hpp"
#include <yaz/zoom.h>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <yaz/diagbib1.h>
#include <yaz/log.h>
#include <yaz/zgdu.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Zoom::Backend {
            friend class Impl;
            friend class Frontend;
            std::string zurl;
            ZOOM_connection m_connection;
            ZOOM_resultset m_resultset;
            std::string m_frontend_database;
        public:
            Backend();
            ~Backend();
            void connect(std::string zurl);
        };
        class Zoom::Frontend {
            friend class Impl;
            Impl *m_p;
            bool m_is_virtual;
            bool m_in_use;
            yazpp_1::GDU m_init_gdu;
            std::list<BackendPtr> m_backend_list;
            void handle_package(mp::Package &package);
            void handle_search(mp::Package &package);
            void handle_present(mp::Package &package);
            BackendPtr get_backend_from_databases(std::string database);
        public:
            Frontend(Impl *impl);
            ~Frontend();
        };
        class Zoom::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
        private:
            FrontendPtr get_frontend(mp::Package &package);
            void release_frontend(mp::Package &package);

            std::map<mp::Session, FrontendPtr> m_clients;            
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
        };
    }
}

// define Pimpl wrapper forwarding to Impl
 
yf::Zoom::Zoom() : m_p(new Impl)
{
}

yf::Zoom::~Zoom()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Zoom::configure(const xmlNode *xmlnode, bool test_only)
{
    m_p->configure(xmlnode);
}

void yf::Zoom::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff

yf::Zoom::Backend::Backend()
{
    m_connection = ZOOM_connection_create(0);
    m_resultset = 0;
}

yf::Zoom::Backend::~Backend()
{
    ZOOM_connection_destroy(m_connection);
    ZOOM_resultset_destroy(m_resultset);
}

void yf::Zoom::Backend::connect(std::string zurl)
{
    ZOOM_connection_connect(m_connection, zurl.c_str(), 0);
}

yf::Zoom::Frontend::Frontend(Impl *impl) : 
    m_p(impl), m_is_virtual(false), m_in_use(true)
{
}

yf::Zoom::Frontend::~Frontend()
{
}

yf::Zoom::FrontendPtr yf::Zoom::Impl::get_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<mp::Session,yf::Zoom::FrontendPtr>::iterator it;
    
    while(true)
    {
        it = m_clients.find(package.session());
        if (it == m_clients.end())
            break;
        
        if (!it->second->m_in_use)
        {
            it->second->m_in_use = true;
            return it->second;
        }
        m_cond_session_ready.wait(lock);
    }
    FrontendPtr f(new Frontend(this));
    m_clients[package.session()] = f;
    f->m_in_use = true;
    return f;
}

void yf::Zoom::Impl::release_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<mp::Session,yf::Zoom::FrontendPtr>::iterator it;
    
    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        if (package.session().is_closed())
        {
            m_clients.erase(it);
        }
        else
        {
            it->second->m_in_use = false;
        }
        m_cond_session_ready.notify_all();
    }
}

yf::Zoom::Impl::Impl()
{
}

yf::Zoom::Impl::~Impl()
{ 
}

void yf::Zoom::Impl::configure(const xmlNode *xmlnode)
{
}

yf::Zoom::BackendPtr yf::Zoom::Frontend::get_backend_from_databases(
    std::string database)
{
    std::list<BackendPtr>::const_iterator map_it;
    map_it = m_backend_list.begin();
    for (; map_it != m_backend_list.end(); map_it++)
        if ((*map_it)->m_frontend_database == database)
            return *map_it;

    BackendPtr b(new Backend);

    std::string url = "localhost:9999/" + database;
    yaz_log(YLOG_LOG, "new backend url=%s", url.c_str());
    b->connect(url);
    return b;
}

void yf::Zoom::Frontend::handle_search(mp::Package &package)
{
    Z_GDU *gdu = package.request().get();
    Z_APDU *apdu_req = gdu->u.z3950;
    Z_APDU *apdu_res = 0;
    mp::odr odr;
    Z_SearchRequest *sr = apdu_req->u.searchRequest;
    if (sr->num_databaseNames != 1)
    {
        apdu_res = odr.create_searchResponse(
            apdu_req, YAZ_BIB1_TOO_MANY_DATABASES_SPECIFIED, 0);
        package.response() = apdu_res;
        return;
    }
    BackendPtr b = get_backend_from_databases(sr->databaseNames[0]);
    switch (sr->query->which)
    {
    case Z_Query_type_1:
    case Z_Query_type_101:
        apdu_res = 
            odr.create_searchResponse(
                apdu_req,
                YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                "search filter do not handle type-1/type-101 yet");
        package.response() = apdu_res;
        break;
    default:
        apdu_res = 
            odr.create_searchResponse(
                apdu_req,
                YAZ_BIB1_QUERY_TYPE_UNSUPP, 0);
        package.response() = apdu_res;
        return;
    }
}

void yf::Zoom::Frontend::handle_present(mp::Package &package)
{
    Z_GDU *gdu = package.request().get();
    Z_APDU *apdu_req = gdu->u.z3950;
    mp::odr odr;
    package.response() = odr.create_close(
        apdu_req,
        Z_Close_protocolError,
        "zoom filter has not implemented present request yet");
    package.session().close();
}

void yf::Zoom::Frontend::handle_package(mp::Package &package)
{
    Z_GDU *gdu = package.request().get();
    if (!gdu)
        ;
    else if (gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu_req = gdu->u.z3950;
        if (apdu_req->which == Z_APDU_initRequest)
        {
            mp::odr odr;
            package.response() = odr.create_close(
                apdu_req,
                Z_Close_protocolError,
                "double init");
        }
        else if (apdu_req->which == Z_APDU_searchRequest)
        {
            handle_search(package);
        }
        else if (apdu_req->which == Z_APDU_presentRequest)
        {
            handle_present(package);
        }
        else
        {
            mp::odr odr;
            package.response() = odr.create_close(
                apdu_req,
                Z_Close_protocolError,
                "zoom filter cannot handle this APDU");
            package.session().close();
        }
    }
    else
    {
        package.session().close();
    }
}

void yf::Zoom::Impl::process(mp::Package &package)
{
    FrontendPtr f = get_frontend(package);
    Z_GDU *gdu = package.request().get();

    if (f->m_is_virtual)
    {
        f->handle_package(package);
    }
    else if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
             Z_APDU_initRequest)
    {
        Z_InitRequest *req = gdu->u.z3950->u.initRequest;
        f->m_init_gdu = gdu;
        
        mp::odr odr;
        Z_APDU *apdu = odr.create_initResponse(gdu->u.z3950, 0, 0);
        Z_InitResponse *resp = apdu->u.initResponse;
        
        int i;
        static const int masks[] = {
            Z_Options_search,
            Z_Options_present,
            -1 
        };
        for (i = 0; masks[i] != -1; i++)
            if (ODR_MASK_GET(req->options, masks[i]))
                ODR_MASK_SET(resp->options, masks[i]);
        
        static const int versions[] = {
            Z_ProtocolVersion_1,
            Z_ProtocolVersion_2,
            Z_ProtocolVersion_3,
            -1
        };
        for (i = 0; versions[i] != -1; i++)
            if (ODR_MASK_GET(req->protocolVersion, versions[i]))
                ODR_MASK_SET(resp->protocolVersion, versions[i]);
            else
                break;
        
        *resp->preferredMessageSize = *req->preferredMessageSize;
        *resp->maximumRecordSize = *req->maximumRecordSize;
        
        package.response() = apdu;
        f->m_is_virtual = true;
    }
    else
        package.move();

    release_frontend(package);
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::Zoom;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_zoom = {
        0,
        "zoom",
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

