/* $Id: filter_virt_db.cpp,v 1.22 2006-01-12 14:45:04 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "util.hpp"
#include "filter_virt_db.hpp"

#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <map>
#include <iostream>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        struct Virt_db::Set {
            Set(yp2::Session &id, std::string setname,
                std::string vhost, std::string route,
                bool named_result_sets);
            Set();
            ~Set();

            yp2::Session m_backend_session;
            std::string m_backend_setname;
            std::string m_vhost;
            std::string m_route;
            bool m_named_result_sets;
        };
        struct Virt_db::Map {
            Map(std::string vhost, std::string route);
            Map();
            std::string m_vhost;
            std::string m_route;
        };
        struct Virt_db::Frontend {
            Frontend();
            ~Frontend();
            yp2::Session m_session;
            bool m_is_virtual;
            bool m_in_use;
            std::map<std::string,Virt_db::Set> m_sets;
            void search(Package &package, Z_APDU *apdu,
                        const std::map<std::string, Virt_db::Map> &maps);
            void present(Package &package, Z_APDU *apdu);
            void close(Package &package);
            typedef std::map<std::string,Virt_db::Set>::iterator Sets_it;
        };            
        class Virt_db::Rep {
            friend class Virt_db;
            
            Frontend *get_frontend(Package &package);
            void release_frontend(Package &package);
        private:
            boost::mutex m_sessions_mutex;
            std::map<std::string, Virt_db::Map>m_maps;

            typedef std::map<std::string,Virt_db::Set>::iterator Sets_it;

            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<yp2::Session,Frontend *> m_clients;
        };
    }
}

using namespace yp2;

yf::Virt_db::Frontend::Frontend()
{
    m_is_virtual = false;
}

void yf::Virt_db::Frontend::close(Package &package)
{
    Sets_it sit = m_sets.begin();
    for (; sit != m_sets.end(); sit++)
    {
        sit->second.m_backend_session.close();
        Package close_package(sit->second.m_backend_session, package.origin());
        close_package.copy_filter(package);
        close_package.move(sit->second.m_route);
    }
    m_sets.clear();
}

yf::Virt_db::Frontend::~Frontend()
{
}

yf::Virt_db::Frontend *yf::Virt_db::Rep::get_frontend(Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<yp2::Session,yf::Virt_db::Frontend *>::iterator it;
    
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
    Frontend *f = new Frontend;
    m_clients[package.session()] = f;
    f->m_in_use = true;
    return f;
}


void yf::Virt_db::Rep::release_frontend(Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<yp2::Session,yf::Virt_db::Frontend *>::iterator it;
    
    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        if (package.session().is_closed())
        {
            it->second->close(package);
            delete it->second;
            m_clients.erase(it);
        }
        else
        {
            it->second->m_in_use = false;
        }
        m_cond_session_ready.notify_all();
    }
}

yf::Virt_db::Set::Set(yp2::Session &id, std::string setname,
                      std::string vhost, std::string route,
                      bool named_result_sets)
    :   m_backend_session(id), m_backend_setname(setname), m_vhost(vhost),
    m_route(route), m_named_result_sets(named_result_sets)
{
}


yf::Virt_db::Set::Set()
{
}


yf::Virt_db::Set::~Set()
{
}

yf::Virt_db::Map::Map(std::string vhost, std::string route)
    : m_vhost(vhost), m_route(route) 
{
}

yf::Virt_db::Map::Map()
{
}

yf::Virt_db::Virt_db() : m_p(new Virt_db::Rep)
{
}

yf::Virt_db::~Virt_db() {
}

void yf::Virt_db::Frontend::present(Package &package, Z_APDU *apdu)
{
    Session *id = 0;
    Z_PresentRequest *req = apdu->u.presentRequest;
    std::string resultSetId = req->resultSetId;
    yp2::odr odr;

    Sets_it sets_it = m_sets.find(resultSetId);
    if (sets_it == m_sets.end())
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
        
        m_sets.erase(resultSetId);
    }
    else
    {
        package.response() = present_package.response();
    }
    delete id;
}

void yf::Virt_db::Frontend::search(Package &package, Z_APDU *apdu,
                          const std::map<std::string, Virt_db::Map> &maps)
{
    Z_SearchRequest *req = apdu->u.searchRequest;
    std::string vhost;
    std::string database;
    std::string resultSetId = req->resultSetName;
    bool support_named_result_sets = false;  // whether backend supports it
    yp2::odr odr;
    
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
    std::map<std::string, Virt_db::Map>::const_iterator map_it;
    map_it = maps.find(database);
    if (map_it == maps.end()) 
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
        Sets_it sets_it = m_sets.find(req->resultSetName);
        if (sets_it != m_sets.end())
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
    m_sets.erase(req->resultSetName);
    vhost = map_it->second.m_vhost;
    std::string route = map_it->second.m_route;
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

        init_package.move(route);  // sending init 

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
    
    search_package.move(route);

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
    
    m_sets[resultSetId] =
        Virt_db::Set(id, backend_resultSetId, vhost, route,
                     support_named_result_sets);
}

void yf::Virt_db::add_map_db2vhost(std::string db, std::string vhost,
                                   std::string route)
{
    m_p->m_maps[db] = Virt_db::Map(vhost, route);
}

void yf::Virt_db::process(Package &package) const
{
    yf::Virt_db::Frontend *f = m_p->get_frontend(package);
    if (f)
    {
        Z_GDU *gdu = package.request().get();
        
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_initRequest && !f->m_is_virtual)
        {
            Z_InitRequest *req = gdu->u.z3950->u.initRequest;

            const char *vhost =
                yaz_oi_get_string_oidval(&req->otherInfo, VAL_PROXY, 1, 0);
            if (!vhost)
            {
                yp2::odr odr;
                Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
                Z_InitResponse *resp = apdu->u.initResponse;
                
                int i;
                static const int masks[] = {
                    Z_Options_search,
                    Z_Options_present,
                    Z_Options_namedResultSets,
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
                
                package.response() = apdu;
                f->m_is_virtual = true;
            }
            else
                package.move();
        }
        else if (!f->m_is_virtual)
            package.move();
        else if (gdu && gdu->which == Z_GDU_Z3950)
        {
            Z_APDU *apdu = gdu->u.z3950;
            if (apdu->which == Z_APDU_initRequest)
            {
                yp2::odr odr;
                
                package.response() = odr.create_close(
                    Z_Close_protocolError,
                    "double init");
                
                package.session().close();
            }
            else if (apdu->which == Z_APDU_searchRequest)
            {
                f->search(package, apdu, m_p->m_maps);
            }
            else if (apdu->which == Z_APDU_presentRequest)
            {
                f->present(package, apdu);
            }
            else
            {
                yp2::odr odr;
                
                package.response() = odr.create_close(
                    Z_Close_protocolError,
                    "unsupported APDU in filter_virt_db");
                
                package.session().close();
            }
        }
    }
    m_p->release_frontend(package);
}


void yp2::filter::Virt_db::configure(const xmlNode * ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "virtual"))
        {
            std::string database;
            std::string target;
            xmlNode *v_node = ptr->children;
            for (; v_node; v_node = v_node->next)
            {
                if (v_node->type != XML_ELEMENT_NODE)
                    continue;
                
                if (yp2::xml::is_element_yp2(v_node, "database"))
                    database = yp2::xml::get_text(v_node);
                else if (yp2::xml::is_element_yp2(v_node, "target"))
                    target = yp2::xml::get_text(v_node);
                else
                    throw yp2::filter::FilterException
                        ("Bad element " 
                         + std::string((const char *) v_node->name)
                         + " in virtual section"
                            );
            }
            std::string route = yp2::xml::get_route(ptr);
            add_map_db2vhost(database, target, route);
            std::cout << "Add " << database << "->" << target
                      << "," << route << "\n";
        }
        else
        {
            throw yp2::filter::FilterException
                ("Bad element " 
                 + std::string((const char *) ptr->name)
                 + " in virt_db filter");
        }
    }
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::Virt_db;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_virt_db = {
        0,
        "virt_db",
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
