/* $Id: filter_virt_db.cpp,v 1.2 2005-10-24 21:01:53 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "filter_virt_db.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <list>
#include <map>
#include <iostream>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        struct Virt_db_set {
            Virt_db_set(yp2::Session &id, Z_InternationalString *setname,
                        std::string vhost);
            ~Virt_db_set();

            yp2::Session m_session;
            std::string m_setname;
            std::string m_vhost;
        };
        struct Virt_db_session {
            Virt_db_session(yp2::Session &id, bool use_vhost);
            yp2::Session m_session;
            bool m_use_vhost;
            std::list<Virt_db_set> m_sets;
        };
        struct Virt_db_map {
            Virt_db_map(std::string vhost);
            Virt_db_map();
            std::string m_vhost;
        };
        class Virt_db::Rep {
            friend class Virt_db;
            
            void release_session(Package &package);
            void init(Package &package, Z_APDU *apdu, bool &move_later);
            void search(Package &package, Z_APDU *apdu, bool &move_later);
        private:
            boost::mutex m_sessions_mutex;
            std::list<Virt_db_session>m_sessions;
            std::map<std::string, Virt_db_map>m_maps;
        };
    }
}

yf::Virt_db_set::Virt_db_set(yp2::Session &id, Z_InternationalString *setname,
                             std::string vhost)
    :   m_session(id), m_setname(setname), m_vhost(vhost)
{
}

yf::Virt_db_set::~Virt_db_set()
{
}

yf::Virt_db_map::Virt_db_map(std::string vhost)
    : m_vhost(vhost) 
{
}

yf::Virt_db_map::Virt_db_map()
{
}

yf::Virt_db_session::Virt_db_session(yp2::Session &id,
                                     bool use_vhost) :
    m_session(id) , m_use_vhost(use_vhost)
{

}

yf::Virt_db::Virt_db() {
    m_p = new Virt_db::Rep;
}

yf::Virt_db::~Virt_db() {
    delete m_p;
}

void yf::Virt_db::Rep::release_session(Package &package)
{
    if (package.session().is_closed()) 
    {
        boost::mutex::scoped_lock lock(m_sessions_mutex);
        
        std::list<Virt_db_session>::iterator it;
        for (it = m_sessions.begin(); it != m_sessions.end(); it++)
        {
            if (package.session() == (*it).m_session)
                break;
        }
        if (it == m_sessions.end())
            return;
        m_sessions.erase(it);
    }
}

void yf::Virt_db::Rep::search(Package &package, Z_APDU *apdu, bool &move_later)
{
    Z_SearchRequest *req = apdu->u.searchRequest;
    std::string vhost;
    std::string database;
    Session *id = 0;
    {
        boost::mutex::scoped_lock lock(m_sessions_mutex);
        
        std::list<Virt_db_session>::iterator it;
        for (it = m_sessions.begin(); it != m_sessions.end(); it++)
        {
            if (package.session() == (*it).m_session)
                break;
        }
        if (it == m_sessions.end())
        {
            // error should be returned
            move_later = true;
            return;
        }
        if ((*it).m_use_vhost)
        {
            move_later = true;
            return;
        }
        if (req->num_databaseNames != 1)
        {   // exactly one database must be specified
            ODR odr = odr_createmem(ODR_ENCODE);
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_TOO_MANY_DATABASES_SPECIFIED, 0);
            package.response() = apdu;
            
            odr_destroy(odr);
            return;
        }
        database = req->databaseNames[0];
        std::map<std::string, Virt_db_map>::iterator map_it;
        map_it = m_maps.find(database);
        if (map_it == m_maps.end()) 
        {   // no map for database: return diagnostic
            ODR odr = odr_createmem(ODR_ENCODE);
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_DATABASE_UNAVAILABLE, database.c_str());
            package.response() = apdu;
            
            odr_destroy(odr);
            return;
        }
        vhost = map_it->second.m_vhost;
        id = new Session;
        (*it).m_sets.push_back(Virt_db_set(*id, req->resultSetName, vhost));
    }
    const char *vhost_cstr = vhost.c_str();
    if (true)
    {  // sending init to backend
        Package init_package(*id, package.origin());
        init_package.copy_filter(package);
        
        ODR odr = odr_createmem(ODR_ENCODE);
        Z_APDU *init_apdu = zget_APDU(odr, Z_APDU_initRequest);
        
        yaz_oi_set_string_oidval(&init_apdu->u.initRequest->otherInfo, odr,
                                 VAL_PROXY, 1, vhost_cstr);
        
        init_package.request() = init_apdu;
        odr_destroy(odr);

        init_package.move();  // send init 

        if (init_package.session().is_closed())
        {
            ODR odr = odr_createmem(ODR_ENCODE);
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_DATABASE_UNAVAILABLE, database.c_str());
            package.response() = apdu;
            
            odr_destroy(odr);
            return;
        }
    }
    // sending search to backend
    Package search_package(*id, package.origin());
    search_package.copy_filter(package);
    const char *sep = strchr(vhost_cstr, '/');
    ODR odr = odr_createmem(ODR_ENCODE);
    if (sep)
        req->databaseNames[0] = odr_strdup(odr, sep+1);
    
    search_package.request() = yazpp_1::GDU(apdu);
    
    odr_destroy(odr);
    
    search_package.move();

    package.response() = search_package.response();
}

void yf::Virt_db::Rep::init(Package &package, Z_APDU *apdu, bool &move_later)
{
    boost::mutex::scoped_lock lock(m_sessions_mutex);
    std::list<Virt_db_session>::iterator it;

    for (it = m_sessions.begin(); it != m_sessions.end(); it++)
    {
        if (package.session() == (*it).m_session)
            break;
    }
    if (it != m_sessions.end())
        m_sessions.erase(it);

    Z_InitRequest *req = apdu->u.initRequest;
    
    const char *vhost =
        yaz_oi_get_string_oidval(&req->otherInfo, VAL_PROXY, 1, 0);
    if (!vhost)
    {
        ODR odr = odr_createmem(ODR_ENCODE);
        
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
        Z_InitResponse *resp = apdu->u.initResponse;
        
        int i;
        static const int masks[] = {
            Z_Options_search, Z_Options_present, 0 
        };
        for (i = 0; masks[i]; i++)
            if (ODR_MASK_GET(req->options, masks[i]))
                ODR_MASK_SET(resp->options, masks[i]);
        
        package.response() = apdu;
        
        odr_destroy(odr);

        m_sessions.push_back(Virt_db_session(package.session(), false));
    }
    else
    {
        m_sessions.push_back(Virt_db_session(package.session(), true));
        move_later = true;
    }
}

void yf::Virt_db::add_map_db2vhost(std::string db, std::string vhost)
{
    m_p->m_maps[db] = Virt_db_map(vhost);
}

void yf::Virt_db::process(Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950)
        package.move();
    else
    {
        bool move_later = false;
        Z_APDU *apdu = gdu->u.z3950;
        if (apdu->which == Z_APDU_initRequest)
        {
            m_p->init(package, apdu, move_later);
        }
        else if (apdu->which == Z_APDU_searchRequest)
        {
            m_p->search(package, apdu, move_later);
        }
        else
        {
            ODR odr = odr_createmem(ODR_ENCODE);
            
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_close);
            
            *apdu->u.close->closeReason = Z_Close_protocolError;
            
            package.response() = apdu;
            package.session().close();
            odr_destroy(odr);
        }
        if (move_later)
            package.move();
    }
    m_p->release_session(package);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
