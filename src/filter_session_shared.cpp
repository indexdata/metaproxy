/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include <metaproxy/util.hpp>
#include "filter_session_shared.hpp"

#include <yaz/copy_types.h>
#include <yaz/log.h>
#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>
#include <yazpp/z-query.h>
#include <yazpp/record-cache.h>
#include <map>
#include <iostream>
#include <time.h>
#include <limits.h>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {

    namespace filter {
        // key for session.. We'll only share sessions with same InitKey
        class SessionShared::InitKey {
        public:
            bool operator < (const SessionShared::InitKey &k) const;
            InitKey(Z_InitRequest *req);
            InitKey(const InitKey &);
            ~InitKey();
        private:
            char *m_idAuthentication_buf;
            int m_idAuthentication_size;
            char *m_otherInfo_buf;
            int m_otherInfo_size;
            ODR m_odr;
        };
        // worker thread .. for expiry of sessions
        class SessionShared::Worker {
        public:
            Worker(SessionShared::Rep *rep);
            void operator() (void);
        private:
            SessionShared::Rep *m_p;
        };
        // backend result set
        class SessionShared::BackendSet {
        public:
            std::string m_result_set_id;
            Databases m_databases;
            Odr_int m_result_set_size;
            yazpp_1::Yaz_Z_Query m_query;
            time_t m_time_last_use;
            void timestamp();
            yazpp_1::RecordCache m_record_cache;

            Z_OtherInformation *additionalSearchInfoRequest;
            Z_OtherInformation *additionalSearchInfoResponse;
            NMEM mem_additionalSearchInfo;
            BackendSet(
                const std::string &result_set_id,
                const Databases &databases,
                const yazpp_1::Yaz_Z_Query &query,
                Z_OtherInformation *additionalSearchInfoRequest);
            ~BackendSet();
            bool search(
                Package &frontend_package,
                Package &search_package,
                const Z_APDU *apdu_req,
                const BackendInstancePtr bp,
                Z_Records **z_records);
        };
        // backend connection instance
        class SessionShared::BackendInstance {
            friend class Rep;
            friend class BackendClass;
            friend class BackendSet;
        public:
            mp::Session m_session;
            BackendSetList m_sets;
            bool m_in_use;
            int m_sequence_this;
            int m_result_set_sequence;
            time_t m_time_last_use;
            mp::Package * m_close_package;
            ~BackendInstance();
            void timestamp();
        };
        // backends of some class (all with same InitKey)
        class SessionShared::BackendClass : boost::noncopyable {
            friend class Rep;
            friend struct Frontend;
            bool m_named_result_sets;
            BackendInstanceList m_backend_list;
            BackendInstancePtr create_backend(const Package &package);
            void remove_backend(BackendInstancePtr b);
            BackendInstancePtr get_backend(const Package &package);
            void use_backend(BackendInstancePtr b);
            void release_backend(BackendInstancePtr b);
            bool expire_instances();
            yazpp_1::GDU m_init_request;
            yazpp_1::GDU m_init_response;
            boost::mutex m_mutex_backend_class;
            int m_sequence_top;
            time_t m_backend_set_ttl;
            time_t m_backend_expiry_ttl;
            size_t m_backend_set_max;
            Odr_int m_preferredMessageSize;
            Odr_int m_maximumRecordSize;
        public:
            BackendClass(const yazpp_1::GDU &init_request,
                         int resultset_ttl,
                         int resultset_max,
                         int session_ttl,
                         Odr_int preferredRecordSize,
                         Odr_int maximumRecordSize);
            ~BackendClass();
        };
        // frontend result set
        class SessionShared::FrontendSet {
            Databases m_databases;
            yazpp_1::Yaz_Z_Query m_query;
        public:
            const Databases &get_databases();
            const yazpp_1::Yaz_Z_Query &get_query();
            FrontendSet(
                const Databases &databases,
                const yazpp_1::Yaz_Z_Query &query);
            FrontendSet();
        };
        // frontend session
        struct SessionShared::Frontend {
            Frontend(Rep *rep);
            ~Frontend();
            bool m_is_virtual;
            bool m_in_use;
            Z_Options m_init_options;
            void search(Package &package, Z_APDU *apdu);
            void present(Package &package, Z_APDU *apdu);
            void scan(Package &package, Z_APDU *apdu);

            int result_set_ref(ODR o,
                               const Databases &databases,
                               Z_RPNStructure *s, std::string &rset);
            void get_set(mp::Package &package,
                         const Z_APDU *apdu_req,
                         const Databases &databases,
                         yazpp_1::Yaz_Z_Query &query,
                         BackendInstancePtr &found_backend,
                         BackendSetPtr &found_set);
            void override_set(BackendInstancePtr &found_backend,
                              std::string &result_set_id,
                              const Databases &databases,
                              bool out_of_sessions);

            Rep *m_p;
            BackendClassPtr m_backend_class;
            FrontendSets m_frontend_sets;
        };
        // representation
        class SessionShared::Rep {
            friend class SessionShared;
            friend struct Frontend;

            FrontendPtr get_frontend(Package &package);
            void release_frontend(Package &package);
            Rep();
        public:
            ~Rep();
            void expire();
        private:
            void expire_classes();
            void stat();
            void init(Package &package, const Z_GDU *gdu,
                      FrontendPtr frontend);
            void start();
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            boost::condition m_cond_expire_ready;
            std::map<mp::Session, FrontendPtr> m_clients;

            BackendClassMap m_backend_map;
            boost::mutex m_mutex_backend_map;
            boost::thread_group m_thrds;
            int m_resultset_ttl;
            int m_resultset_max;
            int m_session_ttl;
            bool m_optimize_search;
            bool m_restart;
            int m_session_max;
            Odr_int m_preferredMessageSize;
            Odr_int m_maximumRecordSize;
            bool close_down;
        };
    }
}

yf::SessionShared::FrontendSet::FrontendSet(
    const Databases &databases,
    const yazpp_1::Yaz_Z_Query &query)
    : m_databases(databases), m_query(query)
{
}

const yf::SessionShared::Databases &
yf::SessionShared::FrontendSet::get_databases()
{
    return m_databases;
}

const yazpp_1::Yaz_Z_Query& yf::SessionShared::FrontendSet::get_query()
{
    return m_query;
}

yf::SessionShared::InitKey::InitKey(const InitKey &k)
{
    m_odr = odr_createmem(ODR_ENCODE);

    m_idAuthentication_size =  k.m_idAuthentication_size;
    m_idAuthentication_buf = (char*)odr_malloc(m_odr, m_idAuthentication_size);
    memcpy(m_idAuthentication_buf, k.m_idAuthentication_buf,
           m_idAuthentication_size);

    m_otherInfo_size =  k.m_otherInfo_size;
    m_otherInfo_buf = (char*)odr_malloc(m_odr, m_otherInfo_size);
    memcpy(m_otherInfo_buf, k.m_otherInfo_buf,
           m_otherInfo_size);
}

yf::SessionShared::InitKey::InitKey(Z_InitRequest *req)
{
    m_odr = odr_createmem(ODR_ENCODE);

    Z_IdAuthentication *t = req->idAuthentication;
    z_IdAuthentication(m_odr, &t, 1, 0);
    m_idAuthentication_buf =
        odr_getbuf(m_odr, &m_idAuthentication_size, 0);

    Z_OtherInformation *o = req->otherInfo;
    z_OtherInformation(m_odr, &o, 1, 0);
    m_otherInfo_buf = odr_getbuf(m_odr, &m_otherInfo_size, 0);
}

yf::SessionShared::InitKey::~InitKey()
{
    odr_destroy(m_odr);
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

void yf::SessionShared::BackendClass::release_backend(BackendInstancePtr b)
{
    boost::mutex::scoped_lock lock(m_mutex_backend_class);
    b->m_in_use = false;
}


void yf::SessionShared::BackendClass::remove_backend(BackendInstancePtr b)
{
    BackendInstanceList::iterator it = m_backend_list.begin();

    while (it != m_backend_list.end())
    {
        if (*it == b)
        {
             mp::odr odr;
            (*it)->m_close_package->response() = odr.create_close(
                0, Z_Close_lackOfActivity, 0);
            (*it)->m_close_package->session().close();
            (*it)->m_close_package->move();

            it = m_backend_list.erase(it);
        }
        else
            it++;
    }
}



yf::SessionShared::BackendInstancePtr
yf::SessionShared::BackendClass::get_backend(
    const mp::Package &frontend_package)
{
    {
        boost::mutex::scoped_lock lock(m_mutex_backend_class);

        BackendInstanceList::const_iterator it = m_backend_list.begin();

        BackendInstancePtr backend1; // null

        for (; it != m_backend_list.end(); it++)
        {
            if (!(*it)->m_in_use)
            {
                if (!backend1
                    || (*it)->m_sequence_this < backend1->m_sequence_this)
                    backend1 = *it;
            }
        }
        if (backend1)
        {
            use_backend(backend1);
            return backend1;
        }
    }
    return create_backend(frontend_package);
}

void yf::SessionShared::BackendClass::use_backend(BackendInstancePtr backend)
{
    backend->m_in_use = true;
    backend->m_sequence_this = m_sequence_top++;
}

void yf::SessionShared::BackendInstance::timestamp()
{
    assert(m_in_use);
    time(&m_time_last_use);
}

yf::SessionShared::BackendInstance::~BackendInstance()
{
    delete m_close_package;
}

yf::SessionShared::BackendInstancePtr yf::SessionShared::BackendClass::create_backend(
    const mp::Package &frontend_package)
{
    BackendInstancePtr bp(new BackendInstance);
    BackendInstancePtr null;

    bp->m_close_package =
        new mp::Package(bp->m_session, frontend_package.origin());
    bp->m_close_package->copy_filter(frontend_package);

    Package init_package(bp->m_session, frontend_package.origin());

    init_package.copy_filter(frontend_package);

    yazpp_1::GDU actual_init_request = m_init_request;
    Z_GDU *init_pdu = actual_init_request.get();

    assert(init_pdu->which == Z_GDU_Z3950);
    assert(init_pdu->u.z3950->which == Z_APDU_initRequest);

    Z_InitRequest *req = init_pdu->u.z3950->u.initRequest;
    ODR_MASK_ZERO(req->options);

    ODR_MASK_SET(req->options, Z_Options_search);
    ODR_MASK_SET(req->options, Z_Options_present);
    ODR_MASK_SET(req->options, Z_Options_namedResultSets);
    ODR_MASK_SET(req->options, Z_Options_scan);

    ODR_MASK_SET(req->protocolVersion, Z_ProtocolVersion_1);
    ODR_MASK_SET(req->protocolVersion, Z_ProtocolVersion_2);
    ODR_MASK_SET(req->protocolVersion, Z_ProtocolVersion_3);

    if (m_preferredMessageSize)
        *req->preferredMessageSize = m_preferredMessageSize;
    if (m_maximumRecordSize)
        *req->maximumRecordSize = m_maximumRecordSize;

    init_package.request() = init_pdu;

    init_package.move();

    boost::mutex::scoped_lock lock(m_mutex_backend_class);

    m_named_result_sets = false;
    Z_GDU *gdu = init_package.response().get();

    if (gdu && gdu->which == Z_GDU_Z3950
        && gdu->u.z3950->which == Z_APDU_initResponse)
    {
        Z_InitResponse *res = gdu->u.z3950->u.initResponse;
        m_init_response = gdu->u.z3950;
        if (ODR_MASK_GET(res->options, Z_Options_namedResultSets))
        {
            m_named_result_sets = true;
        }
        if (*gdu->u.z3950->u.initResponse->result
            && !init_package.session().is_closed())
        {
            bp->m_in_use = true;
            time(&bp->m_time_last_use);
            bp->m_sequence_this = 0;
            bp->m_result_set_sequence = 0;
            m_backend_list.push_back(bp);
            return bp;
        }
    }
    else
    {
        yazpp_1::GDU empty_gdu;
        m_init_response = empty_gdu;
    }

    if (!init_package.session().is_closed())
    {
        init_package.copy_filter(frontend_package);
        init_package.session().close();
        init_package.move();
    }
    return null;
}


yf::SessionShared::BackendClass::BackendClass(const yazpp_1::GDU &init_request,
                                              int resultset_ttl,
                                              int resultset_max,
                                              int session_ttl,
                                              Odr_int preferredMessageSize,
                                              Odr_int maximumRecordSize)
    : m_named_result_sets(false), m_init_request(init_request),
      m_sequence_top(0), m_backend_set_ttl(resultset_ttl),
      m_backend_expiry_ttl(session_ttl), m_backend_set_max(resultset_max),
      m_preferredMessageSize(preferredMessageSize),
      m_maximumRecordSize(maximumRecordSize)
{}

yf::SessionShared::BackendClass::~BackendClass()
{}

void yf::SessionShared::Rep::stat()
{
    int no_classes = 0;
    int no_instances = 0;
    BackendClassMap::const_iterator it;
    {
        boost::mutex::scoped_lock lock(m_mutex_backend_map);
        for (it = m_backend_map.begin(); it != m_backend_map.end(); it++)
        {
            BackendClassPtr bc = it->second;
            no_classes++;
            BackendInstanceList::iterator bit = bc->m_backend_list.begin();
            for (; bit != bc->m_backend_list.end(); bit++)
                no_instances++;
        }
    }
}

void yf::SessionShared::Rep::init(mp::Package &package, const Z_GDU *gdu,
                                  FrontendPtr frontend)
{
    Z_InitRequest *req = gdu->u.z3950->u.initRequest;

    frontend->m_is_virtual = true;
    frontend->m_init_options = *req->options;
    InitKey k(req);
    {
        boost::mutex::scoped_lock lock(m_mutex_backend_map);
        BackendClassMap::const_iterator it;
        it = m_backend_map.find(k);
        if (it == m_backend_map.end())
        {
            BackendClassPtr b(new BackendClass(gdu->u.z3950,
                                               m_resultset_ttl,
                                               m_resultset_max,
                                               m_session_ttl,
                                               m_preferredMessageSize,
                                               m_maximumRecordSize));
            m_backend_map[k] = b;
            frontend->m_backend_class = b;
        }
        else
        {
            frontend->m_backend_class = it->second;
        }
    }
    BackendClassPtr bc = frontend->m_backend_class;
    mp::odr odr;

    // we only need to get init response from "first" target in
    // backend class - the assumption being that init response is
    // same for all
    if (bc->m_backend_list.size() == 0)
    {
        BackendInstancePtr backend = bc->create_backend(package);
        if (backend)
            bc->release_backend(backend);
    }

    yazpp_1::GDU init_response;
    {
        boost::mutex::scoped_lock lock(bc->m_mutex_backend_class);

        init_response = bc->m_init_response;
    }

    if (init_response.get())
    {
        Z_GDU *response_gdu = init_response.get();
        mp::util::transfer_referenceId(odr, gdu->u.z3950,
                                       response_gdu->u.z3950);
        Z_InitResponse *init_res = response_gdu->u.z3950->u.initResponse;
        Z_Options *server_options = init_res->options;
        Z_Options *client_options = &frontend->m_init_options;
        int i;
        for (i = 0; i < 30; i++)
            if (!ODR_MASK_GET(client_options, i))
                ODR_MASK_CLEAR(server_options, i);

        if (!m_preferredMessageSize ||
            *init_res->preferredMessageSize > *req->preferredMessageSize)
            *init_res->preferredMessageSize = *req->preferredMessageSize;

        if (!m_maximumRecordSize ||
            *init_res->maximumRecordSize > *req->maximumRecordSize)
            *init_res->maximumRecordSize = *req->maximumRecordSize;

        package.response() = init_response;
        if (!*response_gdu->u.z3950->u.initResponse->result)
            package.session().close();
    }
    else
    {
        Z_APDU *apdu =
            odr.create_initResponse(
                gdu->u.z3950, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                "session_shared: target closed connection during init");
        *apdu->u.initResponse->result = 0;
        package.response() = apdu;
        package.session().close();
    }
}

void yf::SessionShared::BackendSet::timestamp()
{
    time(&m_time_last_use);
}

yf::SessionShared::BackendSet::BackendSet(
    const std::string &result_set_id,
    const Databases &databases,
    const yazpp_1::Yaz_Z_Query &query,
    Z_OtherInformation *additionalSearchInfo) :
    m_result_set_id(result_set_id),
    m_databases(databases), m_result_set_size(0), m_query(query)
{
    timestamp();
    mem_additionalSearchInfo = nmem_create();
    additionalSearchInfoResponse = 0;
    additionalSearchInfoRequest =
        yaz_clone_z_OtherInformation(additionalSearchInfo,
                                     mem_additionalSearchInfo);
}

yf::SessionShared::BackendSet::~BackendSet()
{
    nmem_destroy(mem_additionalSearchInfo);
}

static int get_diagnostic(Z_DefaultDiagFormat *r)
{
    return *r->condition;
}

bool yf::SessionShared::BackendSet::search(
    mp::Package &frontend_package,
    mp::Package &search_package,
    const Z_APDU *frontend_apdu,
    const BackendInstancePtr bp,
    Z_Records **z_records)
{
    mp::odr odr;
    Z_APDU *apdu_req = zget_APDU(odr, Z_APDU_searchRequest);
    Z_SearchRequest *req = apdu_req->u.searchRequest;

    req->additionalSearchInfo = additionalSearchInfoRequest;
    req->resultSetName = odr_strdup(odr, m_result_set_id.c_str());
    req->query = m_query.get_Z_Query();

    req->num_databaseNames = m_databases.size();
    req->databaseNames = (char**)
        odr_malloc(odr, req->num_databaseNames * sizeof(char *));
    Databases::const_iterator it = m_databases.begin();
    size_t i = 0;
    for (; it != m_databases.end(); it++)
        req->databaseNames[i++] = odr_strdup(odr, it->c_str());

    if (frontend_apdu->which == Z_APDU_searchRequest)
        req->preferredRecordSyntax =
            frontend_apdu->u.searchRequest->preferredRecordSyntax;

    search_package.request() = apdu_req;

    search_package.move();

    Z_GDU *gdu = search_package.response().get();
    if (!search_package.session().is_closed()
        && gdu && gdu->which == Z_GDU_Z3950
        && gdu->u.z3950->which == Z_APDU_searchResponse)
    {
        Z_SearchResponse *b_resp = gdu->u.z3950->u.searchResponse;
        *z_records = b_resp->records;
        m_result_set_size = *b_resp->resultCount;

        additionalSearchInfoResponse = yaz_clone_z_OtherInformation(
            b_resp->additionalSearchInfo, mem_additionalSearchInfo);
        return true;
    }
    Z_APDU *f_apdu = 0;
    const char *addinfo = "session_shared: "
        "target closed connection during search";
    if (frontend_apdu->which == Z_APDU_searchRequest)
        f_apdu = odr.create_searchResponse(
            frontend_apdu, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, addinfo);
    else if (frontend_apdu->which == Z_APDU_presentRequest)
        f_apdu = odr.create_presentResponse(
            frontend_apdu, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, addinfo);
    else
        f_apdu = odr.create_close(
            frontend_apdu, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, addinfo);
    frontend_package.response() = f_apdu;
    return false;
}

void yf::SessionShared::Frontend::override_set(
    BackendInstancePtr &found_backend,
    std::string &result_set_id,
    const Databases &databases,
    bool out_of_sessions)
{
    BackendClassPtr bc = m_backend_class;
    BackendInstanceList::const_iterator it = bc->m_backend_list.begin();
    time_t now;
    time(&now);

    size_t max_sets = bc->m_named_result_sets ? bc->m_backend_set_max : 1;
    for (; it != bc->m_backend_list.end(); it++)
    {
        if (!(*it)->m_in_use)
        {
            BackendSetList::iterator set_it = (*it)->m_sets.begin();
            for (; set_it != (*it)->m_sets.end(); set_it++)
            {
                if ((max_sets > 1 || (*set_it)->m_databases == databases)
                    &&
                    (out_of_sessions ||
                     now < (*set_it)->m_time_last_use ||
                     now - (*set_it)->m_time_last_use >= bc->m_backend_set_ttl))
                {
                    found_backend = *it;
                    result_set_id = (*set_it)->m_result_set_id;
                    found_backend->m_sets.erase(set_it);
                    return;
                }
            }
        }
    }
    for (it = bc->m_backend_list.begin(); it != bc->m_backend_list.end(); it++)
    {
        if (!(*it)->m_in_use && (*it)->m_sets.size() < max_sets)
        {
            found_backend = *it;
            if (bc->m_named_result_sets)
            {
                result_set_id = boost::io::str(
                    boost::format("%1%") %
                    found_backend->m_result_set_sequence);
                found_backend->m_result_set_sequence++;
            }
            else
                result_set_id = "default";
            return;
        }
    }
}

void yf::SessionShared::Frontend::get_set(mp::Package &package,
                                          const Z_APDU *apdu_req,
                                          const Databases &databases,
                                          yazpp_1::Yaz_Z_Query &query,
                                          BackendInstancePtr &found_backend,
                                          BackendSetPtr &found_set)
{
    bool session_restarted = false;
    Z_OtherInformation *additionalSearchInfo = 0;

    if (apdu_req->which == Z_APDU_searchRequest)
        additionalSearchInfo = apdu_req->u.searchRequest->additionalSearchInfo;

restart:
    std::string result_set_id;
    bool out_of_sessions = false;
    BackendClassPtr bc = m_backend_class;
    {
        boost::mutex::scoped_lock lock(bc->m_mutex_backend_class);

        if ((int) bc->m_backend_list.size() >= m_p->m_session_max)
            out_of_sessions = true;

        if (m_p->m_optimize_search)
        {
            // look at each backend and see if we have a similar search
            BackendInstanceList::const_iterator it = bc->m_backend_list.begin();
            for (; it != bc->m_backend_list.end(); it++)
            {
                if (!(*it)->m_in_use)
                {
                    BackendSetList::const_iterator set_it = (*it)->m_sets.begin();
                    for (; set_it != (*it)->m_sets.end(); set_it++)
                    {
                        // for real present request we don't care
                        // if additionalSearchInfo matches: same records
                        if ((*set_it)->m_databases == databases
                            && query.match(&(*set_it)->m_query)
                            && (apdu_req->which != Z_APDU_searchRequest ||
                                yaz_compare_z_OtherInformation(
                                    additionalSearchInfo,
                                (*set_it)->additionalSearchInfoRequest)))
                        {
                            found_set = *set_it;
                            found_backend = *it;
                            bc->use_backend(found_backend);
                            // found matching set. No need to search again
                            return;
                        }
                    }
                }
            }
        }
        override_set(found_backend, result_set_id, databases, out_of_sessions);
        if (found_backend)
            bc->use_backend(found_backend);
    }
    if (!found_backend)
    {
        // create a new backend set (and new set) if we're not out of sessions
        if (!out_of_sessions)
            found_backend = bc->create_backend(package);

        if (!found_backend)
        {
            Z_APDU *f_apdu = 0;
            mp::odr odr;
            const char *addinfo = 0;

            if (out_of_sessions)
                addinfo = "session_shared: all sessions in use";
            else
                addinfo = "session_shared: could not create backend";
            if (apdu_req->which == Z_APDU_searchRequest)
            {
                f_apdu = odr.create_searchResponse(
                    apdu_req, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, addinfo);
            }
            else if (apdu_req->which == Z_APDU_presentRequest)
            {
                f_apdu = odr.create_presentResponse(
                    apdu_req, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, addinfo);
            }
            else
            {
                f_apdu = odr.create_close(
                    apdu_req, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR, addinfo);
            }
            package.response() = f_apdu;
            return;
        }
        if (bc->m_named_result_sets)
        {
            result_set_id = boost::io::str(
                boost::format("%1%") % found_backend->m_result_set_sequence);
            found_backend->m_result_set_sequence++;
        }
        else
            result_set_id = "default";
    }
    found_backend->timestamp();

    // we must search ...
    BackendSetPtr new_set(new BackendSet(result_set_id,
                                         databases, query,
                                         additionalSearchInfo));
    Z_Records *z_records = 0;

    Package search_package(found_backend->m_session, package.origin());
    search_package.copy_filter(package);

    if (!new_set->search(package, search_package,
                         apdu_req, found_backend, &z_records))
    {
        bc->remove_backend(found_backend);
        return; // search error
    }

    if (z_records)
    {
        int condition = 0;
        if (z_records->which == Z_Records_NSD)
        {
            condition =
                get_diagnostic(z_records->u.nonSurrogateDiagnostic);
        }
        else if (z_records->which == Z_Records_multipleNSD)
        {
            if (z_records->u.multipleNonSurDiagnostics->num_diagRecs >= 1
                &&

                z_records->u.multipleNonSurDiagnostics->diagRecs[0]->which ==
                Z_DiagRec_defaultFormat)
            {
                condition = get_diagnostic(
                    z_records->u.multipleNonSurDiagnostics->diagRecs[0]->u.defaultFormat);

            }
        }
        if (m_p->m_restart && !session_restarted &&
            condition == YAZ_BIB1_TEMPORARY_SYSTEM_ERROR)
        {
            package.log("session_shared", YLOG_LOG, "restart");
            bc->remove_backend(found_backend);
            session_restarted = true;
            found_backend.reset();
            goto restart;

        }

        if (condition)
        {
            mp::odr odr;
            if (apdu_req->which == Z_APDU_searchRequest)
            {
                Z_APDU *f_apdu = odr.create_searchResponse(apdu_req,
                                                           0, 0);
                Z_SearchResponse *f_resp = f_apdu->u.searchResponse;
                *f_resp->searchStatus = Z_SearchResponse_none;
                f_resp->records = z_records;
                package.response() = f_apdu;
            }
            if (apdu_req->which == Z_APDU_presentRequest)
            {
                Z_APDU *f_apdu = odr.create_presentResponse(apdu_req,
                                                            0, 0);
                Z_PresentResponse *f_resp = f_apdu->u.presentResponse;
                f_resp->records = z_records;
                package.response() = f_apdu;
            }
            bc->release_backend(found_backend);
            return; // search error
        }
    }
    if (m_p->m_restart && !session_restarted && new_set->m_result_set_size < 0)
    {
        package.log("session_shared", YLOG_LOG, "restart");
        bc->remove_backend(found_backend);
        session_restarted = true;
        found_backend.reset();
        goto restart;
    }

    found_set = new_set;
    found_set->timestamp();
    found_backend->m_sets.push_back(found_set);
}

int yf::SessionShared::Frontend::result_set_ref(ODR o,
                                                const Databases &databases,
                                                Z_RPNStructure *s,
                                                std::string &rset)
{
    int ret = 0;
    switch (s->which)
    {
    case Z_RPNStructure_simple:
        if (s->u.simple->which == Z_Operand_resultSetId)
        {
            const char *id = s->u.simple->u.resultSetId;
            rset = id;

            FrontendSets::iterator fset_it = m_frontend_sets.find(id);
            if (fset_it == m_frontend_sets.end())
            {
                ret = YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST;
            }
            else if (fset_it->second->get_databases() != databases)
            {
                ret = YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST;
            }
            else
            {
                yazpp_1::Yaz_Z_Query query = fset_it->second->get_query();
                Z_Query *q = yaz_copy_Z_Query(query.get_Z_Query(), o);
                if (q->which == Z_Query_type_1 || q->which == Z_Query_type_101)
                {
                    s->which = q->u.type_1->RPNStructure->which;
                    s->u.simple = q->u.type_1->RPNStructure->u.simple;
                }
                else
                {
                    ret = YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST;
                }
            }
        }
        break;
    case Z_RPNStructure_complex:
        ret = result_set_ref(o, databases, s->u.complex->s1, rset);
        if (!ret)
            ret = result_set_ref(o, databases, s->u.complex->s2, rset);
        break;
    }
    return ret;
}

void yf::SessionShared::Frontend::search(mp::Package &package,
                                         Z_APDU *apdu_req)
{
    Z_SearchRequest *req = apdu_req->u.searchRequest;
    FrontendSets::iterator fset_it =
        m_frontend_sets.find(req->resultSetName);
    if (fset_it != m_frontend_sets.end())
    {
        // result set already exist
        // if replace indicator is off: we return diagnostic if
        // result set already exist.
        if (*req->replaceIndicator == 0)
        {
            mp::odr odr;
            Z_APDU *apdu =
                odr.create_searchResponse(
                    apdu_req,
                    YAZ_BIB1_RESULT_SET_EXISTS_AND_REPLACE_INDICATOR_OFF,
                    0);
            package.response() = apdu;
            return;
        }
        m_frontend_sets.erase(fset_it);
    }

    Databases databases;
    int i;
    for (i = 0; i < req->num_databaseNames; i++)
        databases.push_back(req->databaseNames[i]);


    yazpp_1::Yaz_Z_Query query;
    query.set_Z_Query(req->query);

    Z_Query *q = query.get_Z_Query();
    if (q->which == Z_Query_type_1 || q->which == Z_Query_type_101)
    {
        mp::odr odr;
        std::string rset;
        int diag = result_set_ref(odr, databases, q->u.type_1->RPNStructure,
                                  rset);
        if (diag)
        {
            Z_APDU *apdu =
                odr.create_searchResponse(
                    apdu_req,
                    diag,
                    rset.c_str());
            package.response() = apdu;
            return;
        }
        query.set_Z_Query(q);
    }

    BackendSetPtr found_set; // null
    BackendInstancePtr found_backend; // null

    get_set(package, apdu_req, databases, query, found_backend, found_set);
    if (!found_set)
        return;

    mp::odr odr;
    Z_APDU *f_apdu = odr.create_searchResponse(apdu_req, 0, 0);
    Z_SearchResponse *f_resp = f_apdu->u.searchResponse;
    *f_resp->resultCount = found_set->m_result_set_size;
    f_resp->additionalSearchInfo = found_set->additionalSearchInfoResponse;
    package.response() = f_apdu;

    FrontendSetPtr fset(new FrontendSet(databases, query));
    m_frontend_sets[req->resultSetName] = fset;

    m_backend_class->release_backend(found_backend);
}

void yf::SessionShared::Frontend::present(mp::Package &package,
                                          Z_APDU *apdu_req)
{
    mp::odr odr;
    Z_PresentRequest *req = apdu_req->u.presentRequest;

    FrontendSets::iterator fset_it =
        m_frontend_sets.find(req->resultSetId);

    if (fset_it == m_frontend_sets.end())
    {
        Z_APDU *apdu =
            odr.create_presentResponse(
                apdu_req,
                YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
                req->resultSetId);
        package.response() = apdu;
        return;
    }
    FrontendSetPtr fset = fset_it->second;

    Databases databases = fset->get_databases();
    yazpp_1::Yaz_Z_Query query = fset->get_query();

    BackendClassPtr bc = m_backend_class;
    BackendSetPtr found_set; // null
    BackendInstancePtr found_backend;

    get_set(package, apdu_req, databases, query, found_backend, found_set);
    if (!found_set)
        return;

    Z_NamePlusRecordList *npr_res = 0;
    // record_cache.lookup types are int's. Avoid non-fitting values
    if (*req->resultSetStartPoint > 0
        && *req->resultSetStartPoint < INT_MAX
        && *req->numberOfRecordsRequested > 0
        && *req->numberOfRecordsRequested < INT_MAX
        && found_set->m_record_cache.lookup(odr, &npr_res,
                                            *req->resultSetStartPoint,
                                            *req->numberOfRecordsRequested,
                                            req->preferredRecordSyntax,
                                            req->recordComposition))
    {
        Z_APDU *f_apdu_res = odr.create_presentResponse(apdu_req, 0, 0);
        Z_PresentResponse *f_resp = f_apdu_res->u.presentResponse;

        yaz_log(YLOG_LOG, "Found " ODR_INT_PRINTF "+" ODR_INT_PRINTF
                " records in cache %p",
                *req->resultSetStartPoint,
                *req->numberOfRecordsRequested,
                &found_set->m_record_cache);

        *f_resp->numberOfRecordsReturned = *req->numberOfRecordsRequested;
        *f_resp->nextResultSetPosition =
            *req->resultSetStartPoint + *req->numberOfRecordsRequested;
        // f_resp->presentStatus assumed OK.
        f_resp->records = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
        f_resp->records->which = Z_Records_DBOSD;
        f_resp->records->u.databaseOrSurDiagnostics = npr_res;
        package.response() = f_apdu_res;
        bc->release_backend(found_backend);
        return;
    }

    found_backend->timestamp();

    Z_APDU *p_apdu = zget_APDU(odr, Z_APDU_presentRequest);
    Z_PresentRequest *p_req = p_apdu->u.presentRequest;
    p_req->preferredRecordSyntax = req->preferredRecordSyntax;
    p_req->resultSetId = odr_strdup(odr, found_set->m_result_set_id.c_str());
    *p_req->resultSetStartPoint = *req->resultSetStartPoint;
    *p_req->numberOfRecordsRequested = *req->numberOfRecordsRequested;
    p_req->preferredRecordSyntax = req->preferredRecordSyntax;
    p_req->recordComposition = req->recordComposition;

    Package present_package(found_backend->m_session, package.origin());
    present_package.copy_filter(package);

    present_package.request() = p_apdu;

    present_package.move();

    Z_GDU *gdu = present_package.response().get();
    if (!present_package.session().is_closed()
        && gdu && gdu->which == Z_GDU_Z3950
        && gdu->u.z3950->which == Z_APDU_presentResponse)
    {
        Z_PresentResponse *b_resp = gdu->u.z3950->u.presentResponse;
        Z_APDU *f_apdu_res = odr.create_presentResponse(apdu_req, 0, 0);
        Z_PresentResponse *f_resp = f_apdu_res->u.presentResponse;

        f_resp->numberOfRecordsReturned = b_resp->numberOfRecordsReturned;
        f_resp->nextResultSetPosition = b_resp->nextResultSetPosition;
        f_resp->presentStatus= b_resp->presentStatus;
        f_resp->records = b_resp->records;
        f_resp->otherInfo = b_resp->otherInfo;
        package.response() = f_apdu_res;

        if (b_resp->records && b_resp->records->which ==  Z_Records_DBOSD)
        {
            Z_NamePlusRecordList *npr =
                b_resp->records->u.databaseOrSurDiagnostics;
            // record_cache.add types are int's. Avoid non-fitting values
            if (*req->resultSetStartPoint > 0
                && npr->num_records + *req->resultSetStartPoint < INT_MAX)
            {
#if 0
                yaz_log(YLOG_LOG, "Adding " ODR_INT_PRINTF "+" ODR_INT_PRINTF
                        " records to cache %p",
                        *req->resultSetStartPoint,
                        *f_resp->numberOfRecordsReturned,
                        &found_set->m_record_cache);
#endif
                found_set->m_record_cache.add(
                    odr, npr, *req->resultSetStartPoint,
                    p_req->recordComposition);
            }
        }
        bc->release_backend(found_backend);
    }
    else
    {
        bc->remove_backend(found_backend);
        Z_APDU *f_apdu_res =
            odr.create_presentResponse(
                apdu_req, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                "session_shared: target closed connection during present");
        package.response() = f_apdu_res;
    }
}

void yf::SessionShared::Frontend::scan(mp::Package &frontend_package,
                                       Z_APDU *apdu_req)
{
    BackendClassPtr bc = m_backend_class;
    BackendInstancePtr backend = bc->get_backend(frontend_package);
    if (!backend)
    {
        mp::odr odr;
        Z_APDU *apdu = odr.create_scanResponse(
            apdu_req, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
            "session_shared: could not create backend");
        frontend_package.response() = apdu;
    }
    else
    {
        Package scan_package(backend->m_session, frontend_package.origin());
        backend->timestamp();
        scan_package.copy_filter(frontend_package);
        scan_package.request() = apdu_req;
        scan_package.move();
        frontend_package.response() = scan_package.response();
        if (scan_package.session().is_closed())
        {
            frontend_package.session().close();
            bc->remove_backend(backend);
        }
        else
            bc->release_backend(backend);
    }
}

yf::SessionShared::Worker::Worker(SessionShared::Rep *rep) : m_p(rep)
{
}

void yf::SessionShared::Worker::operator() (void)
{
    m_p->expire();
}

bool yf::SessionShared::BackendClass::expire_instances()
{
    time_t now;
    time(&now);
    boost::mutex::scoped_lock lock(m_mutex_backend_class);
    BackendInstanceList::iterator bit = m_backend_list.begin();
    while (bit != m_backend_list.end())
    {
        time_t last_use = (*bit)->m_time_last_use;

        if ((*bit)->m_in_use)
        {
            bit++;
        }
        else if (now < last_use || now - last_use > m_backend_expiry_ttl)
        {
            mp::odr odr;
            (*bit)->m_close_package->response() = odr.create_close(
                0, Z_Close_lackOfActivity, 0);
            (*bit)->m_close_package->session().close();
            (*bit)->m_close_package->move();

            bit = m_backend_list.erase(bit);
        }
        else
        {
            bit++;
        }
    }
    if (m_backend_list.empty())
        return true;
    return false;
}

void yf::SessionShared::Rep::expire_classes()
{
    boost::mutex::scoped_lock lock(m_mutex_backend_map);
    BackendClassMap::iterator b_it = m_backend_map.begin();
    while (b_it != m_backend_map.end())
    {
        if (b_it->second->expire_instances())
        {
            m_backend_map.erase(b_it);
            b_it = m_backend_map.begin();
        }
        else
            b_it++;
    }
}

void yf::SessionShared::Rep::expire()
{
    while (true)
    {
        boost::xtime xt;
        boost::xtime_get(&xt,
#if BOOST_VERSION >= 105000
                boost::TIME_UTC_
#else
                boost::TIME_UTC
#endif
                  );
        xt.sec += m_session_ttl / 3;
        {
            boost::mutex::scoped_lock lock(m_mutex);
            m_cond_expire_ready.timed_wait(lock, xt);
            if (close_down)
                break;
        }
        stat();
        expire_classes();
    }
}

yf::SessionShared::Rep::Rep()
{
    m_resultset_ttl = 30;
    m_resultset_max = 10;
    m_session_ttl = 90;
    m_optimize_search = true;
    m_restart = false;
    m_session_max = 100;
    m_preferredMessageSize = 0;
    m_maximumRecordSize = 0;
    close_down = false;
}

yf::SessionShared::Rep::~Rep()
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
        close_down = true;
        m_cond_expire_ready.notify_all();
    }
    m_thrds.join_all();
}

void yf::SessionShared::Rep::start()
{
    yf::SessionShared::Worker w(this);
    m_thrds.add_thread(new boost::thread(w));
}

yf::SessionShared::SessionShared() : m_p(new SessionShared::Rep)
{
}

yf::SessionShared::~SessionShared() {
}

void yf::SessionShared::start() const
{
    m_p->start();
}

yf::SessionShared::Frontend::Frontend(Rep *rep) : m_is_virtual(false), m_p(rep)
{
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
        m_p->init(package, gdu, f);
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
        else if (apdu->which == Z_APDU_close)
        {
            mp::odr odr;

            package.response() = odr.create_close(
                apdu,
                Z_Close_peerAbort, "received close from client");
            package.session().close();
        }
        else if (apdu->which == Z_APDU_searchRequest)
        {
            f->search(package, apdu);
        }
        else if (apdu->which == Z_APDU_presentRequest)
        {
            f->present(package, apdu);
        }
        else if (apdu->which == Z_APDU_scanRequest)
        {
            f->scan(package, apdu);
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

void yf::SessionShared::configure(const xmlNode *ptr, bool test_only,
                                  const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "resultset"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "ttl"))
                    m_p->m_resultset_ttl =
                        mp::xml::get_int(attr->children, 30);
                else if (!strcmp((const char *) attr->name, "max"))
                {
                    m_p->m_resultset_max =
                        mp::xml::get_int(attr->children, 10);
                }
                else if (!strcmp((const char *) attr->name, "optimizesearch"))
                {
                    m_p->m_optimize_search =
                        mp::xml::get_bool(attr->children, true);
                }
                else if (!strcmp((const char *) attr->name, "restart"))
                {
                    m_p->m_restart = mp::xml::get_bool(attr->children, true);
                }
                else
                    throw mp::filter::FilterException(
                        "Bad attribute " + std::string((const char *)
                                                       attr->name));
            }
        }
        else if (!strcmp((const char *) ptr->name, "session"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "ttl"))
                    m_p->m_session_ttl =
                        mp::xml::get_int(attr->children, 90);
                else if (!strcmp((const char *) attr->name, "max"))
                    m_p->m_session_max =
                        mp::xml::get_int(attr->children, 100);
                else
                    throw mp::filter::FilterException(
                        "Bad attribute " + std::string((const char *)
                                                       attr->name));
            }
        }
        else if (!strcmp((const char *) ptr->name, "init"))
        {
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name, "maximum-record-size"))
                    m_p->m_maximumRecordSize =
                        mp::xml::get_int(attr->children, 0);
                else if (!strcmp((const char *) attr->name,
                                 "preferred-message-size"))
                    m_p->m_preferredMessageSize =
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

