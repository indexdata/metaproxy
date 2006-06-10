/* $Id: filter_session_shared.cpp,v 1.11 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>

#include "util.hpp"
#include "filter_session_shared.hpp"

#include <yaz/log.h>
#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <map>
#include <iostream>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {

    namespace filter {

        class SessionShared::InitKey {
        public:
            bool operator < (const SessionShared::InitKey &k) const;
            InitKey(Z_InitRequest *req);
        private:
            char *m_idAuthentication_buf;
            int m_idAuthentication_size;
            char *m_otherInfo_buf;
            int m_otherInfo_size;
            mp::odr m_odr;

            std::list<std::string> m_targets;
        };
        class SessionShared::BackendClass {
            yazpp_1::GDU m_init_response;
        };
        struct SessionShared::Frontend {
            void init(Package &package, Z_GDU *gdu);
            Frontend(Rep *rep);
            ~Frontend();
            mp::Session m_session;
            bool m_is_virtual;
            bool m_in_use;

            void close(Package &package);
            Rep *m_p;
        };            
        class SessionShared::Rep {
            friend class SessionShared;
            friend struct Frontend;
            
            FrontendPtr get_frontend(Package &package);
            void release_frontend(Package &package);
        private:
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<mp::Session, FrontendPtr> m_clients;

            typedef std::map<InitKey,BackendClass> BackendClassMap;
            BackendClassMap m_backend_map;
        };
    }
}


yf::SessionShared::InitKey::InitKey(Z_InitRequest *req)
{
    Z_IdAuthentication *t = req->idAuthentication;
    
    z_IdAuthentication(m_odr, &t, 1, 0);
    m_idAuthentication_buf =
        odr_getbuf(m_odr, &m_idAuthentication_size, 0);

    Z_OtherInformation *o = req->otherInfo;
    z_OtherInformation(m_odr, &o, 1, 0);
    m_otherInfo_buf = odr_getbuf(m_odr, &m_otherInfo_size, 0);
}

bool yf::SessionShared::InitKey::operator < (const SessionShared::InitKey &k)
    const 
{
    int c;
    c = mp::util::memcmp2(
        (void*) m_idAuthentication_buf, m_idAuthentication_size,
        (void*) k.m_idAuthentication_buf, k.m_idAuthentication_size);
    if (c < 0)
        return true;
    else if (c > 0)
        return false;

    c = mp::util::memcmp2((void*) m_otherInfo_buf, m_otherInfo_size,
                          (void*) k.m_otherInfo_buf, k.m_otherInfo_size);
    if (c < 0)
        return true;
    else if (c > 0)
        return false;
    return false;
}

void yf::SessionShared::Frontend::init(mp::Package &package, Z_GDU *gdu)
{
    Z_InitRequest *req = gdu->u.z3950->u.initRequest;

    std::list<std::string> targets;

    mp::util::get_vhost_otherinfo(&req->otherInfo, false, targets);

    // std::cout << "SessionShared::Frontend::init\n";
    if (targets.size() < 1)
    {
        // no targets given, just relay this one and don't deal with it
        package.move();
        return;
    }
    InitKey k(req);
}

yf::SessionShared::SessionShared() : m_p(new SessionShared::Rep)
{
}

yf::SessionShared::~SessionShared() {
}


yf::SessionShared::Frontend::Frontend(Rep *rep) : m_is_virtual(false), m_p(rep)
{
}

void yf::SessionShared::Frontend::close(mp::Package &package)
{
#if 0
    std::list<BackendPtr>::const_iterator b_it;
    
    for (b_it = m_backend_list.begin(); b_it != m_backend_list.end(); b_it++)
    {
        (*b_it)->m_backend_session.close();
        Package close_package((*b_it)->m_backend_session, package.origin());
        close_package.copy_filter(package);
        close_package.move((*b_it)->m_route);
    }
    m_backend_list.clear();
#endif
}


yf::SessionShared::Frontend::~Frontend()
{
}

yf::SessionShared::FrontendPtr yf::SessionShared::Rep::get_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<mp::Session,yf::SessionShared::FrontendPtr>::iterator it;
    
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

void yf::SessionShared::Rep::release_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<mp::Session,yf::SessionShared::FrontendPtr>::iterator it;
    
    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        if (package.session().is_closed())
        {
            it->second->close(package);
            m_clients.erase(it);
        }
        else
        {
            it->second->m_in_use = false;
        }
        m_cond_session_ready.notify_all();
    }
}


void yf::SessionShared::process(mp::Package &package) const
{
    FrontendPtr f = m_p->get_frontend(package);

    Z_GDU *gdu = package.request().get();
    
    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_initRequest && !f->m_is_virtual)
    {
        f->init(package, gdu);
    }
    else if (!f->m_is_virtual)
        package.move();
    else if (gdu && gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu = gdu->u.z3950;
        if (apdu->which == Z_APDU_initRequest)
        {
            mp::odr odr;
            
            package.response() = odr.create_close(
                apdu,
                Z_Close_protocolError,
                "double init");
            
            package.session().close();
        }
        else
        {
            mp::odr odr;
            
            package.response() = odr.create_close(
                apdu, Z_Close_protocolError,
                "unsupported APDU in filter_session_shared");
            
            package.session().close();
        }
    }
    m_p->release_frontend(package);
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::SessionShared;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_session_shared = {
        0,
        "session_shared",
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
