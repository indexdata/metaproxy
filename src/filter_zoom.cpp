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
#include "torus.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <yaz/diagbib1.h>
#include <yaz/log.h>
#include <yaz/zgdu.h>
#include <yaz/querytowrbuf.h>

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
            void connect(std::string zurl, int *error, const char **addinfo);
            void search_pqf(const char *pqf, Odr_int *hits,
                            int *error, const char **addinfo);
            void set_option(const char *name, const char *value);
            int get_error(const char **addinfo);
        };
        struct Zoom::Searchable {
            std::string m_database;
            std::string m_target;
            std::string query_encoding;
            std::string sru;
            Searchable(std::string norm_db, std::string target);
            ~Searchable();
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
            BackendPtr get_backend_from_databases(std::string &database,
                                                  int *error,
                                                  const char **addinfo);
        public:
            Frontend(Impl *impl);
            ~Frontend();
        };
        class Zoom::Impl {
            friend class Frontend;
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr, bool test_only);
        private:
            FrontendPtr get_frontend(mp::Package &package);
            void release_frontend(mp::Package &package);
            void parse_torus(const xmlNode *ptr);

            std::list<Zoom::Searchable>m_searchables;

            std::map<mp::Session, FrontendPtr> m_clients;            
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            mp::Torus torus;
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
    m_p->configure(xmlnode, test_only);
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

void yf::Zoom::Backend::connect(std::string zurl,
                                int *error, const char **addinfo)
{
    ZOOM_connection_connect(m_connection, zurl.c_str(), 0);
    *error = ZOOM_connection_error(m_connection, 0, addinfo);
    yaz_log(YLOG_LOG, "ZOOM_connection_connect: error: %d", *error);
}

void yf::Zoom::Backend::search_pqf(const char *pqf, Odr_int *hits,
                                   int *error, const char **addinfo)
{
    yaz_log(YLOG_LOG, "ZOOM_connection_search_pqf pqf=%s", pqf);
    m_resultset = ZOOM_connection_search_pqf(m_connection, pqf);
    *error = ZOOM_connection_error(m_connection, 0, addinfo);
    yaz_log(YLOG_LOG, "ZOOM_connection_search_pqf: error: %d", *error);
    if (*error == 0)
        *hits = ZOOM_resultset_size(m_resultset);
    else
        *hits = 0;
}

void yf::Zoom::Backend::set_option(const char *name, const char *value)
{
    ZOOM_connection_option_set(m_connection, name, value);
}

int yf::Zoom::Backend::get_error(const char **addinfo)
{
    return ZOOM_connection_error(m_connection, 0, addinfo);
}

yf::Zoom::Searchable::Searchable(std::string database, 
                                 std::string target)
    : m_database(database), m_target(target)
{
}

yf::Zoom::Searchable::~Searchable()
{
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

void yf::Zoom::Impl::parse_torus(const xmlNode *ptr1)
{
    if (!ptr1)
        return ;
    for (ptr1 = ptr1->children; ptr1; ptr1 = ptr1->next)
    {
        if (ptr1->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr1->name, "record"))
        {
            const xmlNode *ptr2 = ptr1;
            for (ptr2 = ptr2->children; ptr2; ptr2 = ptr2->next)
            {
                if (ptr2->type != XML_ELEMENT_NODE)
                    continue;
                if (!strcmp((const char *) ptr2->name, "layer"))
                {
                    std::string database;
                    std::string target;
                    std::string route;
                    std::string sru;
                    std::string query_encoding;
                    const xmlNode *ptr3 = ptr2;
                    for (ptr3 = ptr3->children; ptr3; ptr3 = ptr3->next)
                    {
                        if (ptr3->type != XML_ELEMENT_NODE)
                            continue;
                        if (!strcmp((const char *) ptr3->name, "id"))
                        {
                            database = mp::xml::get_text(ptr3);
                        }
                        else if (!strcmp((const char *) ptr3->name, "zurl"))
                        {
                            target = mp::xml::get_text(ptr3);
                        }
                        else if (!strcmp((const char *) ptr3->name, "sru"))
                        {
                            sru = mp::xml::get_text(ptr3);
                        }
                        else if (!strcmp((const char *) ptr3->name,
                                         "queryEncoding"))
                        {
                            query_encoding = mp::xml::get_text(ptr3);
                        }
                    }
                    if (database.length() && target.length())
                    {
                        yaz_log(YLOG_LOG, "add db=%s target=%s", 
                                database.c_str(), target.c_str());
                        Zoom::Searchable searchable(
                            mp::util::database_name_normalize(database),
                            target);
                        searchable.query_encoding = query_encoding;
                        searchable.sru = sru;
                        m_searchables.push_back(searchable);
                    }
                }
            }
        }
    }
}


void yf::Zoom::Impl::configure(const xmlNode *ptr, bool test_only)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "records"))
        {
            parse_torus(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "torus"))
        {
            std::string url;
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "url"))
                    url = mp::xml::get_text(attr->children);
                else
                    throw mp::filter::FilterException(
                        "Bad attribute " + std::string((const char *)
                                                       attr->name));
            }
            torus.read_searchables(url);
            xmlDoc *doc = torus.get_doc();
            if (doc)
            {
                xmlNode *ptr = xmlDocGetRootElement(doc);
                parse_torus(ptr);
            }
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element " 
                 + std::string((const char *) ptr->name)
                 + " in zoom filter");
        }
    }
}

yf::Zoom::BackendPtr yf::Zoom::Frontend::get_backend_from_databases(
    std::string &database, int *error, const char **addinfo)
{
    std::list<BackendPtr>::const_iterator map_it;
    map_it = m_backend_list.begin();
    for (; map_it != m_backend_list.end(); map_it++)
        if ((*map_it)->m_frontend_database == database)
            return *map_it;

    std::list<Zoom::Searchable>::const_iterator map_s =
        m_p->m_searchables.begin();

    std::string c_db = mp::util::database_name_normalize(database);

    while (map_s != m_p->m_searchables.end())
    {
        yaz_log(YLOG_LOG, "consider db=%s map db=%s",
                database.c_str(), map_s->m_database.c_str());
        if (c_db.compare(map_s->m_database) == 0)
            break;
        map_s++;
    }
    if (map_s == m_p->m_searchables.end())
    {
        *error = YAZ_BIB1_DATABASE_DOES_NOT_EXIST;
        *addinfo = database.c_str();
        BackendPtr b;
        return b;
    }
    BackendPtr b(new Backend);

    if (map_s->query_encoding.length())
        b->set_option("rpnCharset", map_s->query_encoding.c_str());

    std::string url;
    if (map_s->sru.length())
    {
        url = "http://" + map_s->m_target;
        b->set_option("sru", map_s->sru.c_str());
    }
    else
        url = map_s->m_target;

    b->connect(url, error, addinfo);
    if (*error == 0)
    {
        m_backend_list.push_back(b);
    }
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

    int error;
    const char *addinfo;
    std::string db(sr->databaseNames[0]);
    BackendPtr b = get_backend_from_databases(db, &error, &addinfo);
    if (error)
    {
        apdu_res = 
            odr.create_searchResponse(
                apdu_req, error, addinfo);
        package.response() = apdu_res;
        return;
    }
    Z_Query *query = sr->query;
    if (query->which == Z_Query_type_1 || query->which == Z_Query_type_101)
    {
        WRBUF w = wrbuf_alloc();
        yaz_rpnquery_to_wrbuf(w, query->u.type_1);
        Odr_int hits;
        int error;
        const char *addinfo = 0;

        b->search_pqf(wrbuf_cstr(w), &hits, &error, &addinfo);
        wrbuf_destroy(w);

        apdu_res = 
            odr.create_searchResponse(
                apdu_req, error, addinfo);
        apdu_res->u.searchResponse->resultCount = odr_intdup(odr, hits);
        package.response() = apdu_res;
    }
    else
    {
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

