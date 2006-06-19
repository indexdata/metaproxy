/* $Id: filter_session_shared.cpp,v 1.12 2006-06-19 23:54:02 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>

#include "util.hpp"
#include "filter_session_shared.hpp"

#include <yaz/log.h>
#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>
#include <yazpp/z-query.h>
#include <map>
#include <iostream>
#include <time.h>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {

    namespace filter {
        class SessionShared::InitKey {
        public:
            bool operator < (const SessionShared::InitKey &k) const;
            InitKey(Z_InitRequest *req);
            InitKey(const InitKey &);
            ~InitKey();
        private:
            InitKey &operator = (const InitKey &k);
            char *m_idAuthentication_buf;
            int m_idAuthentication_size;
            char *m_otherInfo_buf;
            int m_otherInfo_size;
            ODR m_odr;
        };
        class SessionShared::Worker {
        public:
            Worker(SessionShared::Rep *rep);
            void operator() (void);
        private:
            SessionShared::Rep *m_p;
        };
        class SessionShared::BackendSet {
        public:
            std::string m_result_set_id;
            Databases m_databases;
            int m_result_set_size;
            yazpp_1::Yaz_Z_Query m_query;
            time_t m_time_last_use;
            void timestamp();
            BackendSet(
                const std::string &result_set_id,
                const Databases &databases,
                const yazpp_1::Yaz_Z_Query &query);
            bool search(
                Package &frontend_package,
                const Z_APDU *apdu_req,
                const BackendInstancePtr bp);
        };
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
        };
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
            void expire();
            yazpp_1::GDU m_init_request;
            yazpp_1::GDU m_init_response;
            boost::mutex m_mutex_backend_class;
            int m_sequence_top;
            time_t m_backend_set_ttl;
            time_t m_backend_expiry_ttl;
            size_t m_backend_set_max;
        public:
            BackendClass(const yazpp_1::GDU &init_request);
            ~BackendClass();
        };
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
        struct SessionShared::Frontend {
            Frontend(Rep *rep);
            ~Frontend();
            bool m_is_virtual;
            bool m_in_use;

            void search(Package &package, Z_APDU *apdu);
            void present(Package &package, Z_APDU *apdu);

            void get_set(mp::Package &package,
                         const Z_APDU *apdu_req,
                         const Databases &databases,
                         yazpp_1::Yaz_Z_Query &query,
                         BackendInstancePtr &found_backend,
                         BackendSetPtr &found_set);
            void override_set(BackendInstancePtr &found_backend,
                              std::string &result_set_id);

            Rep *m_p;
            BackendClassPtr m_backend_class;
            FrontendSets m_frontend_sets;
        };            
        class SessionShared::Rep {
            friend class SessionShared;
            friend struct Frontend;
            
            FrontendPtr get_frontend(Package &package);
            void release_frontend(Package &package);
            Rep();
        public:
            void expire();
        private:
            void init(Package &package, const Z_GDU *gdu,
                      FrontendPtr frontend);
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<mp::Session, FrontendPtr> m_clients;

            BackendClassMap m_backend_map;
            boost::mutex m_mutex_backend_map;
            boost::thread_group m_thrds;
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
            it = m_backend_list.erase(it);
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
    time(&backend->m_time_last_use);
    backend->m_sequence_this = m_sequence_top++;
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

    init_package.request() = m_init_request;

    init_package.move();

    boost::mutex::scoped_lock lock(m_mutex_backend_class);

    m_named_result_sets = false;
    Z_GDU *gdu = init_package.response().get();
    if (!init_package.session().is_closed()
        && gdu && gdu->which == Z_GDU_Z3950 
        && gdu->u.z3950->which == Z_APDU_initResponse)
    {
        Z_InitResponse *res = gdu->u.z3950->u.initResponse;
        if (!*res->result)
            return null;
        m_init_response = gdu->u.z3950;
        if (ODR_MASK_GET(res->options, Z_Options_namedResultSets))
        {
            m_named_result_sets = true;
        }
    }
    else
    {
        // did not receive an init response or closed
        return null;
    }
    bp->m_in_use = true;
    time(&bp->m_time_last_use);
    bp->m_sequence_this = 0;
    bp->m_result_set_sequence = 0;
    m_backend_list.push_back(bp);

    return bp;
}


yf::SessionShared::BackendClass::BackendClass(const yazpp_1::GDU &init_request)
    : m_named_result_sets(false), m_init_request(init_request),
      m_sequence_top(0), m_backend_set_ttl(30),
      m_backend_expiry_ttl(90), m_backend_set_max(10)
{}

yf::SessionShared::BackendClass::~BackendClass()
{}

void yf::SessionShared::Rep::init(mp::Package &package, const Z_GDU *gdu,
                                  FrontendPtr frontend)
{
    Z_InitRequest *req = gdu->u.z3950->u.initRequest;

    frontend->m_is_virtual = true;
    InitKey k(req);
    {
        boost::mutex::scoped_lock lock(m_mutex_backend_map);
        BackendClassMap::const_iterator it;
        it = m_backend_map.find(k);
        if (it == m_backend_map.end())
        {
            BackendClassPtr b(new BackendClass(gdu->u.z3950));
            m_backend_map[k] = b;
            frontend->m_backend_class = b;
            std::cout << "SessionShared::Rep::init new session " 
                      << frontend->m_backend_class << "\n";
        }
        else
        {
            frontend->m_backend_class = it->second;            
            std::cout << "SessionShared::Rep::init existing session "
                      << frontend->m_backend_class << "\n";
        }
    }
    BackendClassPtr bc = frontend->m_backend_class;
    BackendInstancePtr backend = bc->get_backend(package);
    
    mp::odr odr;
    if (!backend)
    {
        Z_APDU *apdu = odr.create_initResponse(gdu->u.z3950, 0, 0);
        *apdu->u.initResponse->result = 0;
        package.response() = apdu;
        package.session().close();
    }
    else
    {
        boost::mutex::scoped_lock lock(bc->m_mutex_backend_class);
        Z_GDU *response_gdu = bc->m_init_response.get();
        mp::util::transfer_referenceId(odr, gdu->u.z3950,
                                       response_gdu->u.z3950);
        package.response() = response_gdu;
    }
    if (backend)
        bc->release_backend(backend);
}

void yf::SessionShared::BackendSet::timestamp()
{
    time(&m_time_last_use);
}

yf::SessionShared::BackendSet::BackendSet(
    const std::string &result_set_id,
    const Databases &databases,
    const yazpp_1::Yaz_Z_Query &query) :
    m_result_set_id(result_set_id),
    m_databases(databases), m_result_set_size(0), m_query(query) 
{
    timestamp();
}

bool yf::SessionShared::BackendSet::search(
    Package &frontend_package,
    const Z_APDU *frontend_apdu,
    const BackendInstancePtr bp)
{
    Package search_package(bp->m_session, frontend_package.origin());

    search_package.copy_filter(frontend_package);

    mp::odr odr;
    Z_APDU *apdu_req = zget_APDU(odr, Z_APDU_searchRequest);
    Z_SearchRequest *req = apdu_req->u.searchRequest;

    req->resultSetName = odr_strdup(odr, m_result_set_id.c_str());
    req->query = m_query.get_Z_Query();

    req->num_databaseNames = m_databases.size();
    req->databaseNames = (char**) 
        odr_malloc(odr, req->num_databaseNames * sizeof(char *));
    Databases::const_iterator it = m_databases.begin();
    size_t i = 0;
    for (; it != m_databases.end(); it++)
        req->databaseNames[i++] = odr_strdup(odr, it->c_str());

    search_package.request() = apdu_req;

    search_package.move();
    
    Z_Records *z_records_diag = 0;
    Z_GDU *gdu = search_package.response().get();
    if (!search_package.session().is_closed()
        && gdu && gdu->which == Z_GDU_Z3950 
        && gdu->u.z3950->which == Z_APDU_searchResponse)
    {
        Z_SearchResponse *b_resp = gdu->u.z3950->u.searchResponse;
        if (b_resp->records)
        {
            if (b_resp->records->which == Z_Records_NSD 
                || b_resp->records->which == Z_Records_multipleNSD)
                z_records_diag = b_resp->records;
        }
        if (z_records_diag)
        {
            if (frontend_apdu->which == Z_APDU_searchRequest)
            {
                Z_APDU *f_apdu = odr.create_searchResponse(frontend_apdu, 
                                                           0, 0);
                Z_SearchResponse *f_resp = f_apdu->u.searchResponse;
                f_resp->records = z_records_diag;
                frontend_package.response() = f_apdu;
                return false;
            }
            if (frontend_apdu->which == Z_APDU_presentRequest)
            {
                Z_APDU *f_apdu = odr.create_presentResponse(frontend_apdu, 
                                                            0, 0);
                Z_PresentResponse *f_resp = f_apdu->u.presentResponse;
                f_resp->records = z_records_diag;
                frontend_package.response() = f_apdu;
                return false;
            }
        }
        m_result_set_size = *b_resp->resultCount;
        return true;
    }
    if (frontend_apdu->which == Z_APDU_searchRequest)
    {
        Z_APDU *f_apdu = 
            odr.create_searchResponse(frontend_apdu, 1, "Search closed");
        frontend_package.response() = f_apdu;
    }
    if (frontend_apdu->which == Z_APDU_presentRequest)
    {
        Z_APDU *f_apdu = 
            odr.create_presentResponse(frontend_apdu, 1, "Search closed");
        frontend_package.response() = f_apdu;
    }
    return false;
}

void yf::SessionShared::Frontend::override_set(
    BackendInstancePtr &found_backend,
    std::string &result_set_id)
{
    BackendClassPtr bc = m_backend_class;
    BackendInstanceList::const_iterator it = bc->m_backend_list.begin();
    time_t now;
    time(&now);
    
    for (; it != bc->m_backend_list.end(); it++)
    {
        if (!(*it)->m_in_use)
        {
            BackendSetList::iterator set_it = (*it)->m_sets.begin();
            for (; set_it != (*it)->m_sets.end(); set_it++)
            {
                if (now >= (*set_it)->m_time_last_use &&
                    now - (*set_it)->m_time_last_use > bc->m_backend_set_ttl)
                {
                    found_backend = *it;
                    result_set_id = (*set_it)->m_result_set_id;
                    found_backend->m_sets.erase(set_it);
                    std::cout << "REUSE TTL SET: " << result_set_id << "\n";
                    return;
                }
            }
        }
    }
    size_t max_sets = bc->m_named_result_sets ? bc->m_backend_set_max : 1;
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
            std::cout << "AVAILABLE SET: " << result_set_id << "\n";
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
    std::string result_set_id;
    BackendClassPtr bc = m_backend_class;
    {
        boost::mutex::scoped_lock lock(bc->m_mutex_backend_class);
     
        // look at each backend and see if we have a similar search
        BackendInstanceList::const_iterator it = bc->m_backend_list.begin();
        
        for (; it != bc->m_backend_list.end(); it++)
        {
            if (!(*it)->m_in_use)
            {
                BackendSetList::const_iterator set_it = (*it)->m_sets.begin();
                for (; set_it != (*it)->m_sets.end(); set_it++)
                {
                    if ((*set_it)->m_databases == databases 
                        && query.match(&(*set_it)->m_query))
                    {
                        found_set = *set_it;
                        found_backend = *it;
                        bc->use_backend(found_backend);
                        found_set->timestamp();
                        std::cout << "MATCH SET: " << 
                            found_set->m_result_set_id << "\n";
                        // found matching set. No need to search again
                        return;
                    }
                }
            }
        }
        override_set(found_backend, result_set_id);
        if (found_backend)
            bc->use_backend(found_backend);
    }
    if (!found_backend)
    {
        // create a new backend set (and new set)
        found_backend = bc->create_backend(package);
        assert(found_backend);
        std::cout << "NEW " << found_backend << "\n";
        
        if (bc->m_named_result_sets)
        {
            result_set_id = boost::io::str(
                boost::format("%1%") % found_backend->m_result_set_sequence);
            found_backend->m_result_set_sequence++;
        }
        else
            result_set_id = "default";
        std::cout << "NEW SET: " << result_set_id << "\n";
    }
    // we must search ...
    BackendSetPtr new_set(new BackendSet(result_set_id,
                                         databases, query));
    if (!new_set->search(package, apdu_req, found_backend))
    {
        std::cout << "search error\n";
        bc->release_backend(found_backend);
        return; // search error 
    }
    found_set = new_set;
    found_set->timestamp();
    found_backend->m_sets.push_back(found_set);
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
    
    yazpp_1::Yaz_Z_Query query;
    query.set_Z_Query(req->query);
    Databases databases;
    int i;
    for (i = 0; i<req->num_databaseNames; i++)
        databases.push_back(req->databaseNames[i]);

    BackendSetPtr found_set; // null
    BackendInstancePtr found_backend; // null

    get_set(package, apdu_req, databases, query, found_backend, found_set);

    if (!found_set)
        return;
    mp::odr odr;
    Z_APDU *f_apdu = odr.create_searchResponse(apdu_req, 0, 0);
    Z_SearchResponse *f_resp = f_apdu->u.searchResponse;
    *f_resp->resultCount = found_set->m_result_set_size;
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
        bc->release_backend(found_backend);
    }
    else
    {
        bc->remove_backend(found_backend);
        Z_APDU *f_apdu_res = 
            odr.create_presentResponse(apdu_req, 1, "present error");
        package.response() = f_apdu_res;
    }
}

yf::SessionShared::Worker::Worker(SessionShared::Rep *rep) : m_p(rep)
{
}

void yf::SessionShared::Worker::operator() (void)
{
    m_p->expire();
}

void yf::SessionShared::BackendClass::expire()
{
    time_t now;
    time(&now);
    boost::mutex::scoped_lock lock(m_mutex_backend_class);
    BackendInstanceList::iterator bit = m_backend_list.begin();
    while (bit != m_backend_list.end())
    {
        std::cout << "expiry ";
        time_t last_use = (*bit)->m_time_last_use;
        if ((now >= last_use && now - last_use > m_backend_expiry_ttl)
            || (now < last_use))
        {
            mp::odr odr;
            (*bit)->m_close_package->response() = odr.create_close(
                0, Z_Close_lackOfActivity, "session expired");
            (*bit)->m_close_package->session().close();
            (*bit)->m_close_package->move();

            bit = m_backend_list.erase(bit);
            std::cout << "erase";
        }
        else
        {
            std::cout << "keep";
            bit++;
        }
        std::cout << std::endl;
    }
}

void yf::SessionShared::Rep::expire()
{
    while (true)
    {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 30;
        boost::thread::sleep(xt);
        std::cout << "." << std::endl;
        
        BackendClassMap::const_iterator b_it = m_backend_map.begin();
        for (; b_it != m_backend_map.end(); b_it++)
            b_it->second->expire();
    }
}

yf::SessionShared::Rep::Rep()
{
    yf::SessionShared::Worker w(this);
    m_thrds.add_thread(new boost::thread(w));
}

yf::SessionShared::SessionShared() : m_p(new SessionShared::Rep)
{
}

yf::SessionShared::~SessionShared() {
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
