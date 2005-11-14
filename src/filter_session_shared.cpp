/* $Id: filter_session_shared.cpp,v 1.1 2005-11-14 23:35:22 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>

#include "util.hpp"
#include "filter_session_shared.hpp"

#include <yaz/log.h>
#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <map>
#include <iostream>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class Session_shared::Rep {
            friend class Session_shared;
            void handle_init(Z_InitRequest *req, Package &package);
            void handle_search(Z_SearchRequest *req, Package &package);
        public:
            typedef boost::shared_ptr<Session_shared::List> SharedList;

            typedef std::map<Session_shared::InitKey,SharedList> InitListMap;
            InitListMap m_init_list_map;

            typedef std::map<Session,SharedList> SessionListMap;
            SessionListMap m_session_list_map;

        };
        class Session_shared::InitKey {
            friend class Session_shared;
            friend class Session_shared::Rep;
            std::string m_vhost;
            std::string m_open;
            std::string m_user;
            std::string m_group;
            std::string m_password;
        public:
            bool operator < (const Session_shared::InitKey &k) const;
        };
        class Session_shared::List {
        public:
            yazpp_1::GDU m_init_response;  // init response for backend 
            Session m_session;             // session for backend
        };
    }
    
}


using namespace yp2;

bool yf::Session_shared::InitKey::operator < (const Session_shared::InitKey &k) const {
    if (m_vhost < k.m_vhost)
        return true;
    else if (m_vhost < k.m_vhost)
        return false;

    if (m_open < k.m_open)
        return true;
    else if (m_open > k.m_open)
        return false;

    if (m_user < k.m_user)
        return true;
    else if (m_user > k.m_user)
        return false;
 
    if (m_group < k.m_group)
        return true;
    else if (m_group > k.m_group)
        return false;

    if (m_password < k.m_password)
        return true;
    else if (m_password > k.m_password)
        return false;
    return false;
}

yf::Session_shared::Session_shared() : m_p(new Rep)
{
}

yf::Session_shared::~Session_shared()
{
}

void yf::Session_shared::Rep::handle_search(Z_SearchRequest *req,
                                            Package &package)
{
    yaz_log(YLOG_LOG, "Got search");

    SessionListMap::iterator it = m_session_list_map.find(package.session());
    if (it == m_session_list_map.end())
    {
        yp2::odr odr;
        package.response() = odr.create_close(
                Z_Close_protocolError,
                "no session for search request in session_shared");
        package.session().close();
        
        return;
    }
    Package search_package(it->second->m_session, package.origin());
    search_package.copy_filter(package);
    search_package.request() = package.request();
    
    search_package.move();
        
    // transfer to frontend
    package.response() = search_package.response();
}

void yf::Session_shared::Rep::handle_init(Z_InitRequest *req, Package &package)
{
    yaz_log(YLOG_LOG, "Got init");

    Session_shared::InitKey key;
    const char *vhost =
        yaz_oi_get_string_oidval(&req->otherInfo, VAL_PROXY, 1, 0);
    if (vhost)
        key.m_vhost = vhost;

    if (!req->idAuthentication)
    {
        yaz_log(YLOG_LOG, "No authentication");
    }
    else
    {
        Z_IdAuthentication *auth = req->idAuthentication;
        switch(auth->which)
        {
        case Z_IdAuthentication_open:
            yaz_log(YLOG_LOG, "open auth open=%s", auth->u.open);
            key.m_open = auth->u.open;
            break;
        case Z_IdAuthentication_idPass:
            yaz_log(YLOG_LOG, "idPass user=%s group=%s pass=%s",
                    auth->u.idPass->userId, auth->u.idPass->groupId,
                    auth->u.idPass->password);
            if (auth->u.idPass->userId)
                key.m_user = auth->u.idPass->userId;
            if (auth->u.idPass->groupId)
                key.m_group  = auth->u.idPass->groupId;
            if (auth->u.idPass->password)
                key.m_password  = auth->u.idPass->password;
            break;
        case Z_IdAuthentication_anonymous:
            yaz_log(YLOG_LOG, "anonymous");
            break;
        default:
            yaz_log(YLOG_LOG, "other");
        } 
    }
    InitListMap::iterator it = m_init_list_map.find(key);
    if (it == m_init_list_map.end())
    {
        yaz_log(YLOG_LOG, "New KEY");

        // building new package with original init and new session 
        SharedList l(new Session_shared::List);  // new session for backend

        Package init_package(l->m_session, package.origin());
        init_package.copy_filter(package);
        init_package.request() = package.request();

        init_package.move();
        
        // transfer to frontend
        package.response() = init_package.response();
         
        // check that we really got Z39.50 Init Response
        Z_GDU *gdu = init_package.response().get();
        if (gdu && gdu->which == Z_GDU_Z3950
           && gdu->u.z3950->which == Z_APDU_initResponse)
        {
            // save the init response
            l->m_init_response = init_package.response();
            
            // save session and init response for later
            m_init_list_map[key] = l;

            m_session_list_map[package.session()] = l;
        }
    }
    else
    {
        yaz_log(YLOG_LOG, "Existing KEY");
        package.response() = it->second->m_init_response;

        m_session_list_map[package.session()] = it->second;
    }
}

void yf::Session_shared::process(Package &package) const
{
    // don't tell the backend if the "fronent" filter closes..
    // we want to keep them alive
    if (package.session().is_closed())
    {
        m_p->m_session_list_map.erase(package.session());
        return;
    }

    Z_GDU *gdu = package.request().get();

    if (gdu && gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu = gdu->u.z3950;

        switch(apdu->which)
        {
        case Z_APDU_initRequest:
            m_p->handle_init(apdu->u.initRequest, package);
            break;
        case Z_APDU_searchRequest:
            m_p->handle_search(apdu->u.searchRequest, package);
            break;
        default:
            yp2::odr odr;
            package.response() = odr.create_close(
                Z_Close_protocolError,
                "cannot handle a package of this type");
            package.session().close();
            break;
            
        }
        if (package.session().is_closed()) {
            m_p->m_session_list_map.erase(package.session());
        }
    }
    else
        package.move();  // Not Z39.50 or not Init
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
