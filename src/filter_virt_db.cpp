/* $Id: filter_virt_db.cpp,v 1.12 2005-10-30 18:51:20 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "util.hpp"
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
            Virt_db_set(yp2::Session &id, std::string setname,
                        std::string vhost, bool named_result_sets);
            Virt_db_set();
            ~Virt_db_set();

            yp2::Session m_backend_session;
            std::string m_backend_setname;
            std::string m_vhost;
            bool m_named_result_sets;
        };
        struct Virt_db_session {
            Virt_db_session(yp2::Session &id, bool use_vhost);
            Virt_db_session();
            yp2::Session m_session;
            bool m_use_vhost;
            std::map<std::string,Virt_db_set> m_sets;
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
            void present(Package &package, Z_APDU *apdu, bool &move_later);
        private:
            boost::mutex m_sessions_mutex;
            std::map<yp2::Session,Virt_db_session>m_sessions;
            std::map<std::string, Virt_db_map>m_maps;

            typedef std::map<yp2::Session,Virt_db_session>::iterator Ses_it;
            typedef std::map<std::string,Virt_db_set>::iterator Sets_it;
        };
    }
}

yf::Virt_db_set::Virt_db_set(yp2::Session &id, std::string setname,
                             std::string vhost, bool named_result_sets)
    :   m_backend_session(id), m_backend_setname(setname), m_vhost(vhost),
        m_named_result_sets(named_result_sets)
{
}


yf::Virt_db_set::Virt_db_set()
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

yf::Virt_db_session::Virt_db_session()
    : m_use_vhost(false)
{

}

yf::Virt_db_session::Virt_db_session(yp2::Session &id,
                                     bool use_vhost) :
    m_session(id) , m_use_vhost(use_vhost)
{

}

yf::Virt_db::Virt_db() : m_p(new Virt_db::Rep)
{
}

yf::Virt_db::~Virt_db() {
}

void yf::Virt_db::Rep::release_session(Package &package)
{
    boost::mutex::scoped_lock lock(m_sessions_mutex);
    
    m_sessions.erase(package.session());
}

void yf::Virt_db::Rep::present(Package &package, Z_APDU *apdu, bool &move_later){
    Session *id = 0;
    Z_PresentRequest *req = apdu->u.presentRequest;
    std::string resultSetId = req->resultSetId;
    yp2::odr odr;
    {
        boost::mutex::scoped_lock lock(m_sessions_mutex);
        
        Ses_it it = m_sessions.find(package.session());
        if (it == m_sessions.end())
        {
            package.response() = odr.create_close(
                Z_Close_protocolError,
                "no session for present request");
            package.session().close();
            return;
        }
        if (it->second.m_use_vhost)
        {
            move_later = true;
            return;
        }
        Sets_it sets_it = it->second.m_sets.find(resultSetId);
        if (sets_it == it->second.m_sets.end())
        {
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_presentResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.presentResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr,
                    YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
                    resultSetId.c_str());
            package.response() = apdu;

            return;
        }
        id = new yp2::Session(sets_it->second.m_backend_session);
    }
    
    // sending present to backend
    Package present_package(*id, package.origin());
    present_package.copy_filter(package);
    
    req->resultSetId = odr_strdup(odr, "default");
    present_package.request() = yazpp_1::GDU(apdu);

    present_package.move();

    if (present_package.session().is_closed())
    {
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_presentResponse);
        
        Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
        apdu->u.presentResponse->records = rec;
        rec->which = Z_Records_NSD;
        rec->u.nonSurrogateDiagnostic =
            zget_DefaultDiagFormat(
                odr,
                YAZ_BIB1_RESULT_SET_NO_LONGER_EXISTS_UNILATERALLY_DELETED_BY_,
                resultSetId.c_str());
        package.response() = apdu;
        
        boost::mutex::scoped_lock lock(m_sessions_mutex);
        Ses_it it = m_sessions.find(package.session());
        if (it != m_sessions.end())
            it->second.m_sets.erase(resultSetId);
    }
    else
    {
        package.response() = present_package.response();
    }
    delete id;
}

void yf::Virt_db::Rep::search(Package &package, Z_APDU *apdu, bool &move_later)
{
    Z_SearchRequest *req = apdu->u.searchRequest;
    std::string vhost;
    std::string database;
    std::string resultSetId = req->resultSetName;
    bool support_named_result_sets = false;  // whether backend supports it
    yp2::odr odr;
    {
        boost::mutex::scoped_lock lock(m_sessions_mutex);

        Ses_it it = m_sessions.find(package.session());
        if (it == m_sessions.end())
        {
            package.response() = odr.create_close(
                Z_Close_protocolError,
                "no session for search request");
            package.session().close();

            return;
        }
        if (it->second.m_use_vhost)
        {
            move_later = true;
            return;
        }
        if (req->num_databaseNames != 1)
        {   // exactly one database must be specified
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_TOO_MANY_DATABASES_SPECIFIED, 0);
            package.response() = apdu;
            
            return;
        }
        database = req->databaseNames[0];
        std::map<std::string, Virt_db_map>::iterator map_it;
        map_it = m_maps.find(database);
        if (map_it == m_maps.end()) 
        {   // no map for database: return diagnostic
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_DATABASE_DOES_NOT_EXIST, database.c_str());
            package.response() = apdu;
            
            return;
        }
        if (*req->replaceIndicator == 0)
        {
            Sets_it sets_it = it->second.m_sets.find(req->resultSetName);
            if (sets_it != it->second.m_sets.end())
            {
                Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
                
                Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
                apdu->u.searchResponse->records = rec;
                rec->which = Z_Records_NSD;
                rec->u.nonSurrogateDiagnostic =
                    zget_DefaultDiagFormat(
                        odr,
                        YAZ_BIB1_RESULT_SET_EXISTS_AND_REPLACE_INDICATOR_OFF,
                        0);
                package.response() = apdu;
                
                return;
            }
        }
        it->second.m_sets.erase(req->resultSetName);
        vhost = map_it->second.m_vhost;
    }
    // we might look for an existing session with same vhost
    Session id;
    const char *vhost_cstr = vhost.c_str();
    if (true)
    {  // sending init to backend
        Package init_package(id, package.origin());
        init_package.copy_filter(package);
        
        Z_APDU *init_apdu = zget_APDU(odr, Z_APDU_initRequest);
        
        yaz_oi_set_string_oidval(&init_apdu->u.initRequest->otherInfo, odr,
                                 VAL_PROXY, 1, vhost_cstr);
        
        init_package.request() = init_apdu;

        init_package.move();  // sending init 

        if (init_package.session().is_closed())
        {
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_DATABASE_UNAVAILABLE, database.c_str());
            package.response() = apdu;
        }
        Z_GDU *gdu = init_package.response().get();
        // we hope to get an init response
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_initResponse)
        {
            if (ODR_MASK_GET(gdu->u.z3950->u.initResponse->options,
                             Z_Options_namedResultSets))
                support_named_result_sets = true;
        }
        else
        {
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_DATABASE_UNAVAILABLE, database.c_str());
            package.response() = apdu;
            
            return;
        }
    }
    // sending search to backend
    Package search_package(id, package.origin());

    search_package.copy_filter(package);
    const char *sep = strchr(vhost_cstr, '/');
    if (sep)
        req->databaseNames[0] = odr_strdup(odr, sep+1);

    *req->replaceIndicator = 1;

    std::string backend_resultSetId = "default";
    req->resultSetName = odr_strdup(odr, backend_resultSetId.c_str());
    search_package.request() = yazpp_1::GDU(apdu);
    
    search_package.move();

    if (search_package.session().is_closed())
    {
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
        
        Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
        apdu->u.searchResponse->records = rec;
        rec->which = Z_Records_NSD;
        rec->u.nonSurrogateDiagnostic =
            zget_DefaultDiagFormat(
                odr, YAZ_BIB1_DATABASE_UNAVAILABLE, database.c_str());
        package.response() = apdu;
        
        return;
    }
    package.response() = search_package.response();
    
    boost::mutex::scoped_lock lock(m_sessions_mutex);
    Ses_it it = m_sessions.find(package.session());
    if (it != m_sessions.end())
        it->second.m_sets[resultSetId] =
            Virt_db_set(id, backend_resultSetId, vhost,
                        support_named_result_sets);
}

void yf::Virt_db::Rep::init(Package &package, Z_APDU *apdu, bool &move_later)
{
    release_session(package);
    boost::mutex::scoped_lock lock(m_sessions_mutex);

    Z_InitRequest *req = apdu->u.initRequest;
    
    const char *vhost =
        yaz_oi_get_string_oidval(&req->otherInfo, VAL_PROXY, 1, 0);
    if (!vhost)
    {
        yp2::odr odr;
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
        Z_InitResponse *resp = apdu->u.initResponse;
        
        int i;
        static const int masks[] = {
            Z_Options_search, Z_Options_present, Z_Options_namedResultSets, -1 
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

        package.response() = apdu;
        
        m_sessions[package.session()] = Virt_db_session(package.session(), false);
    }
    else
    {
        m_sessions[package.session()] = Virt_db_session(package.session(), true);
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
        else if (apdu->which == Z_APDU_presentRequest)
        {
            m_p->present(package, apdu, move_later);
        }
        else
        {
            yp2::odr odr;
            
            package.response() = odr.create_close(
                Z_Close_protocolError,
                "unsupported APDU in filter_virt_db");
                                                 
            package.session().close();
        }
        if (move_later)
            package.move();
    }
    if (package.session().is_closed())
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
