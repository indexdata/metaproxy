/* $Id: filter_virt_db.cpp,v 1.1 2005-10-24 14:33:30 adam Exp $
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
#include <iostream>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        struct Virt_db_session {
            Virt_db_session(yp2::Session &id, bool use_vhost);
            yp2::Session m_session;
            bool m_use_vhost;
        };
        struct Virt_db_map {
            Virt_db_map(std::string database, std::string vhost);
            std::string m_database;
            std::string m_vhost;
        };
        class Virt_db::Rep {
            friend class Virt_db;
            
            void release_session(Package &package);
            void init(Package &package, Z_APDU *apdu);
            void search(Package &package, Z_APDU *apdu);
        private:
            std::list<Virt_db_session>m_sessions;
            std::list<Virt_db_map>m_maps;
        };
    }
}

yf::Virt_db_map::Virt_db_map(std::string database, std::string vhost)
    : m_database(database), m_vhost(vhost) 
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
    
}

void yf::Virt_db::Rep::search(Package &package, Z_APDU *apdu)
{
    Z_SearchRequest *req = apdu->u.searchRequest;

    std::list<Virt_db_session>::iterator it;
    for (it = m_sessions.begin(); it != m_sessions.end(); it++)
    {
        if (package.session() == (*it).m_session)
            break;
    }
    if (it == m_sessions.end())
        return;
    if ((*it).m_use_vhost)
        package.move();
    else
    {
        if (req->num_databaseNames != 1)
        {
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
        }
        const char *database = req->databaseNames[0];
    }
}

void yf::Virt_db::Rep::init(Package &package, Z_APDU *apdu)
{
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
        package.move();
    }
}

void yf::Virt_db::add_map_db2vhost(std::string db, std::string vhost)
{
    m_p->m_maps.push_back(Virt_db_map(db, vhost));
}

void yf::Virt_db::process(Package &package) const
{
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950)
        package.move();
    else
    {
        Z_APDU *apdu = gdu->u.z3950;
        if (apdu->which == Z_APDU_initRequest)
        {
            m_p->init(package, apdu);
        }
        else if (apdu->which == Z_APDU_searchRequest)
        {
            m_p->search(package, apdu);
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
