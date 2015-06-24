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

#include <yaz/log.h>

#include "config.hpp"

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>

#include <metaproxy/util.hpp>
#include "filter_multi.hpp"

#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>
#include <yaz/match_glob.h>
#include <yaz/oid_db.h>

#include <vector>
#include <algorithm>
#include <map>
#include <iostream>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        enum multi_merge_type {
            round_robin,
            serve_order
        };
        struct Multi::BackendSet {
            BackendPtr m_backend;
            int m_count;
            bool operator < (const BackendSet &k) const;
            bool operator == (const BackendSet &k) const;
        };
        struct Multi::ScanTermInfo {
            std::string m_norm_term;
            std::string m_display_term;
            int m_count;
            bool operator < (const ScanTermInfo &) const;
            bool operator == (const ScanTermInfo &) const;
            Z_Entry *get_entry(ODR odr);
        };
        struct Multi::FrontendSet {
            class PresentJob {
            public:
                BackendPtr m_backend;
                int m_pos; // position for backend (1=first, 2=second,..
                int m_start; // present request start
                PresentJob(BackendPtr ptr, int pos) :
                    m_backend(ptr), m_pos(pos), m_start(0) {};
            };
            FrontendSet(std::string setname);
            FrontendSet();
            ~FrontendSet();

            void round_robin(int pos, int number, std::list<PresentJob> &job);
            void serve_order(int pos, int number, std::list<PresentJob> &job);

            std::list<BackendSet> m_backend_sets;
            std::string m_setname;
        };
        struct Multi::Backend {
            PackagePtr m_package;
            std::string m_backend_database;
            std::string m_vhost;
            std::string m_route;
            std::string m_auth;
            void operator() (void);  // thread operation
        };
        struct Multi::Frontend {
            Frontend(Rep *rep);
            ~Frontend();
            bool m_is_multi;
            bool m_in_use;
            std::list<BackendPtr> m_backend_list;
            std::map<std::string,Multi::FrontendSet> m_sets;

            void multi_move(std::list<BackendPtr> &blist);
            void init(Package &package, Z_GDU *gdu);
            void close(Package &package);
            void search(Package &package, Z_APDU *apdu);
            void present(Package &package, Z_APDU *apdu);
            void scan(Package &package, Z_APDU *apdu);
            void relay_apdu(Package &package, Z_APDU *apdu);
            void record_diagnostics(Z_Records *records, 
                                    Z_DiagRecs * &z_diag,
                                    ODR odr,
                                    int &no_successful);
            Rep *m_p;
        };
        class Multi::Map {
            std::string m_target_pattern;
            std::string m_route;
            std::string m_auth;
        public:
            Map(std::string pattern, std::string route, std::string auth) :
                m_target_pattern(pattern), m_route(route), m_auth(auth) {};
            bool match(const std::string target, std::string *ret,
                       std::string *auth) const {
                if (yaz_match_glob(m_target_pattern.c_str(), target.c_str()))
                {
                    *ret = m_route;
                    *auth = m_auth;
                    return true;
                }
                return false;
            };
        };
        class Multi::Rep {
            friend class Multi;
            friend struct Frontend;

            Rep();
            FrontendPtr get_frontend(Package &package);
            void release_frontend(Package &package);
        private:
            std::list<Multi::Map> m_route_patterns;
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<mp::Session, FrontendPtr> m_clients;
            bool m_hide_unavailable;
            bool m_hide_errors;
            multi_merge_type m_merge_type;
        };
    }
}

yf::Multi::Rep::Rep()
{
    m_hide_unavailable = false;
    m_hide_errors = false;
    m_merge_type = round_robin;
}

bool yf::Multi::BackendSet::operator < (const BackendSet &k) const
{
    return m_count < k.m_count;
}

yf::Multi::Frontend::Frontend(Rep *rep)
{
    m_p = rep;
    m_is_multi = false;
}

yf::Multi::Frontend::~Frontend()
{
}

yf::Multi::FrontendPtr yf::Multi::Rep::get_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<mp::Session,yf::Multi::FrontendPtr>::iterator it;

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

void yf::Multi::Rep::release_frontend(mp::Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<mp::Session,yf::Multi::FrontendPtr>::iterator it;

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

yf::Multi::FrontendSet::FrontendSet(std::string setname)
    :  m_setname(setname)
{
}


yf::Multi::FrontendSet::FrontendSet()
{
}


yf::Multi::FrontendSet::~FrontendSet()
{
}

yf::Multi::Multi() : m_p(new Multi::Rep)
{
}

yf::Multi::~Multi() {
}


void yf::Multi::Backend::operator() (void)
{
    m_package->move(m_route);
}


void yf::Multi::Frontend::close(mp::Package &package)
{
    std::list<BackendPtr>::const_iterator bit;
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        BackendPtr b = *bit;

        b->m_package->copy_filter(package);
        b->m_package->request() = (Z_GDU *) 0;
        b->m_package->session().close();
        b->m_package->move(b->m_route);
    }
}

void yf::Multi::Frontend::multi_move(std::list<BackendPtr> &blist)
{
    std::list<BackendPtr>::const_iterator bit;
    boost::thread_group g;
    for (bit = blist.begin(); bit != blist.end(); bit++)
    {
        g.add_thread(new boost::thread(**bit));
    }
    g.join_all();
}

void yf::Multi::FrontendSet::serve_order(int start, int number,
                                         std::list<PresentJob> &jobs)
{
    int i;
    for (i = 0; i < number; i++)
    {
        std::list<BackendSet>::const_iterator bsit;
        int voffset = 0;
        int offset = start + i - 1;
        for (bsit = m_backend_sets.begin(); bsit != m_backend_sets.end();
             bsit++)
        {
            if (offset >= voffset && offset < voffset + bsit->m_count)
            {
                PresentJob job(bsit->m_backend, offset - voffset + 1);
                jobs.push_back(job);
                break;
            }
            voffset += bsit->m_count;
        }
    }
}

void yf::Multi::FrontendSet::round_robin(int start, int number,
                                         std::list<PresentJob> &jobs)
{
    std::list<int> pos;
    std::list<BackendSet>::const_iterator bsit;
    for (bsit = m_backend_sets.begin(); bsit != m_backend_sets.end(); bsit++)
    {
        pos.push_back(1);
    }

    int p = 1;
#if 1
    // optimization step!
    int omin = 0;
    while(true)
    {
        int min = 0;
        int no_left = 0;
        // find min count for each set which is > omin
        for (bsit = m_backend_sets.begin(); bsit != m_backend_sets.end(); bsit++)
        {
            if (bsit->m_count > omin)
            {
                if (no_left == 0 || bsit->m_count < min)
                    min = bsit->m_count;
                no_left++;
            }
        }
        if (no_left == 0) // if nothing greater than omin, bail out.
            break;
        int skip = no_left * min;
        if (p + skip > start)  // step gets us "into" present range?
        {
            // Yes. skip until start.. Rounding off is deliberate!
            min = (start-p) / no_left;
            p += no_left * min;

            // update positions in each set..
            std::list<int>::iterator psit = pos.begin();
            for (psit = pos.begin(); psit != pos.end(); psit++)
                *psit += min;
            break;
        }
        // skip on each set.. before "present range"..
        p = p + skip;

        std::list<int>::iterator psit = pos.begin();
        for (psit = pos.begin(); psit != pos.end(); psit++)
            *psit += min;

        omin = min; // update so we consider next class (with higher count)
    }
#endif
    int fetched = 0;
    bool more = true;
    while (more)
    {
        more = false;
        std::list<int>::iterator psit = pos.begin();
        bsit = m_backend_sets.begin();

        for (; bsit != m_backend_sets.end(); psit++,bsit++)
        {
            if (fetched >= number)
            {
                more = false;
                break;
            }
            if (*psit <= bsit->m_count)
            {
                if (p >= start)
                {
                    PresentJob job(bsit->m_backend, *psit);
                    jobs.push_back(job);
                    fetched++;
                }
                (*psit)++;
                p++;
                more = true;
            }
        }
    }
}

void yf::Multi::Frontend::init(mp::Package &package, Z_GDU *gdu)
{
    Z_InitRequest *req = gdu->u.z3950->u.initRequest;

    std::list<std::string> targets;

    mp::util::get_vhost_otherinfo(req->otherInfo, targets);

    if (targets.size() < 1)
    {
        package.move();
        return;
    }

    std::list<std::string>::const_iterator t_it = targets.begin();
    for (; t_it != targets.end(); t_it++)
    {
        Session s;
        Backend *b = new Backend;
        b->m_vhost = *t_it;

        std::list<Multi::Map>::const_iterator it =
            m_p->m_route_patterns.begin();
        while (it != m_p->m_route_patterns.end()) {
            if (it->match(*t_it, &b->m_route, &b->m_auth))
                break;
            it++;
        }
        // b->m_route = m_p->m_target_route[*t_it];
        // b->m_route unset
        b->m_package = PackagePtr(new Package(s, package.origin()));

        m_backend_list.push_back(BackendPtr(b));
    }
    m_is_multi = true;

    // create init request
    std::list<BackendPtr>::iterator bit;
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        mp::odr odr;
        BackendPtr b = *bit;
        Z_APDU *init_apdu = zget_APDU(odr, Z_APDU_initRequest);

        std::list<std::string>vhost_one;
        vhost_one.push_back(b->m_vhost);
        mp::util::set_vhost_otherinfo(&init_apdu->u.initRequest->otherInfo,
                                       odr, vhost_one);


        Z_InitRequest *breq = init_apdu->u.initRequest;

        if (b->m_auth.length())
        {
            breq->idAuthentication =
                (Z_IdAuthentication *)
                odr_malloc(odr, sizeof(*breq->idAuthentication));
            breq->idAuthentication->which = Z_IdAuthentication_open;
            breq->idAuthentication->u.open = odr_strdup(odr, b->m_auth.c_str());
        }
        else
            breq->idAuthentication = req->idAuthentication;

        *breq->preferredMessageSize = *req->preferredMessageSize;
        *breq->maximumRecordSize = *req->maximumRecordSize;


        const char *peer_name = yaz_oi_get_string_oid(
            &req->otherInfo, yaz_oid_userinfo_client_ip, 1, 0);
        if (peer_name)
            yaz_oi_set_string_oid(&breq->otherInfo, odr,
                                  yaz_oid_userinfo_client_ip, 1, peer_name);

        ODR_MASK_SET(breq->options, Z_Options_search);
        ODR_MASK_SET(breq->options, Z_Options_present);
        ODR_MASK_SET(breq->options, Z_Options_namedResultSets);
        ODR_MASK_SET(breq->options, Z_Options_scan);

        ODR_MASK_SET(breq->protocolVersion, Z_ProtocolVersion_1);
        ODR_MASK_SET(breq->protocolVersion, Z_ProtocolVersion_2);
        ODR_MASK_SET(breq->protocolVersion, Z_ProtocolVersion_3);

        b->m_package->request() = init_apdu;

        b->m_package->copy_filter(package);
    }
    multi_move(m_backend_list);

    // create the frontend init response based on each backend init response
    mp::odr odr;

    Z_APDU *f_apdu = odr.create_initResponse(gdu->u.z3950, 0, 0);
    Z_InitResponse *f_resp = f_apdu->u.initResponse;

    ODR_MASK_SET(f_resp->options, Z_Options_search);
    ODR_MASK_SET(f_resp->options, Z_Options_present);
    ODR_MASK_SET(f_resp->options, Z_Options_namedResultSets);
    ODR_MASK_SET(f_resp->options, Z_Options_scan);

    ODR_MASK_SET(f_resp->protocolVersion, Z_ProtocolVersion_1);
    ODR_MASK_SET(f_resp->protocolVersion, Z_ProtocolVersion_2);
    ODR_MASK_SET(f_resp->protocolVersion, Z_ProtocolVersion_3);

    int no_failed = 0;
    int no_succeeded = 0;

    Odr_int preferredMessageSize = *req->preferredMessageSize;
    Odr_int maximumRecordSize = *req->maximumRecordSize;
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); )
    {
        PackagePtr p = (*bit)->m_package;

        if (p->session().is_closed())
        {
            // failed. Remove from list and increment number of failed
            no_failed++;
            bit = m_backend_list.erase(bit);
            continue;
        }
        Z_GDU *gdu = p->response().get();
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_initResponse)
        {
            int i;
            Z_APDU *b_apdu = gdu->u.z3950;
            Z_InitResponse *b_resp = b_apdu->u.initResponse;

            // common options for all backends
            for (i = 0; i <= Z_Options_stringSchema; i++)
            {
                if (!ODR_MASK_GET(b_resp->options, i))
                    ODR_MASK_CLEAR(f_resp->options, i);
            }
            // common protocol version
            for (i = 0; i <= Z_ProtocolVersion_3; i++)
                if (!ODR_MASK_GET(b_resp->protocolVersion, i))
                    ODR_MASK_CLEAR(f_resp->protocolVersion, i);
            if (*b_resp->result)
            {
                no_succeeded++;
                if (preferredMessageSize > *b_resp->preferredMessageSize)
                    preferredMessageSize = *b_resp->preferredMessageSize;
                if (maximumRecordSize > *b_resp->maximumRecordSize)
                    maximumRecordSize = *b_resp->maximumRecordSize;
            }
            else
            {
                if (!f_resp->userInformationField
                    && b_resp->userInformationField)
                    f_resp->userInformationField = b_resp->userInformationField;
                no_failed++;
            }
        }
        else
            no_failed++;
        bit++;
    }
    *f_resp->preferredMessageSize = preferredMessageSize;
    *f_resp->maximumRecordSize = maximumRecordSize;

    if (m_p->m_hide_unavailable)
    {
        if (no_succeeded == 0)
        {
            *f_resp->result = 0;
            package.session().close();
        }
    }
    else
    {
        if (no_failed)
        {
            *f_resp->result = 0;
            package.session().close();
        }
    }
    package.response() = f_apdu;
}

void yf::Multi::Frontend::record_diagnostics(Z_Records *records, 
                                             Z_DiagRecs * &z_diag,
                                             ODR odr,
                                             int &no_successful)
{
    // see we get any errors (AKA diagnstics)
    if (records)
    {
        if (records->which == Z_Records_NSD)
        {
            if (!z_diag)
            {
                z_diag = (Z_DiagRecs *)
                    odr_malloc(odr, sizeof(*z_diag));
                z_diag->num_diagRecs = 0;
                z_diag->diagRecs = (Z_DiagRec**)
                    odr_malloc(odr, sizeof(*z_diag->diagRecs));
            }
            else
            {
                Z_DiagRec **n = (Z_DiagRec **)
                    odr_malloc(odr,
                               (1 + z_diag->num_diagRecs) * sizeof(*n));
                memcpy(n, z_diag->diagRecs, z_diag->num_diagRecs
                       * sizeof(*n));
                z_diag->diagRecs = n;
            }
            Z_DiagRec *nr = (Z_DiagRec *) odr_malloc(odr, sizeof(*nr));
            nr->which = Z_DiagRec_defaultFormat;
            nr->u.defaultFormat =
                records->u.nonSurrogateDiagnostic;
            z_diag->diagRecs[z_diag->num_diagRecs++] = nr;
        }
        else if (records->which == Z_Records_multipleNSD)
        {
            Z_DiagRecs * dr =records->u.multipleNonSurDiagnostics;

            if (!z_diag)
            {
                z_diag = (Z_DiagRecs *) odr_malloc(odr, sizeof(*z_diag));
                z_diag->num_diagRecs = 0;
                z_diag->diagRecs = 0;
            }
            Z_DiagRec **n = (Z_DiagRec **)
                odr_malloc(odr,
                           (dr->num_diagRecs + z_diag->num_diagRecs) * 
                           sizeof(*n));
            if (z_diag->num_diagRecs)
                memcpy(n, z_diag->diagRecs, z_diag->num_diagRecs * sizeof(*n));
            memcpy(n + z_diag->num_diagRecs,
                   dr->diagRecs, dr->num_diagRecs * sizeof(*n));
            z_diag->diagRecs = n;
            z_diag->num_diagRecs += dr->num_diagRecs;
        }
        else
            no_successful++; // probably piggyback
    }
    else
        no_successful++;  // no records and no diagnostics
}

void yf::Multi::Frontend::search(mp::Package &package, Z_APDU *apdu_req)
{
    // create search request
    Z_SearchRequest *req = apdu_req->u.searchRequest;

    std::list<BackendPtr>::const_iterator bit;
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        PackagePtr p = (*bit)->m_package;
        yazpp_1::GDU gdu1(apdu_req);
        mp::odr odr;
        Z_SearchRequest *req1 = gdu1.get()->u.z3950->u.searchRequest;

        // they are altered now - to disable piggyback
        *req1->smallSetUpperBound = 0;
        *req1->largeSetLowerBound = 1;
        *req1->mediumSetPresentNumber = 0;

        if (!mp::util::set_databases_from_zurl(odr, (*bit)->m_vhost,
                                               &req1->num_databaseNames,
                                               &req1->databaseNames))
        {
            req1->num_databaseNames = req->num_databaseNames;
            req1->databaseNames = req->databaseNames;
        }
        p->request() = gdu1;
        p->copy_filter(package);
    }
    multi_move(m_backend_list);

    // look at each response
    FrontendSet resultSet(std::string(req->resultSetName));

    mp::odr odr;
    Odr_int result_set_size = 0;
    Z_DiagRecs *z_diag = 0;
    int no_successful = 0;
    PackagePtr close_p;

    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        PackagePtr p = (*bit)->m_package;

        // save closing package for at least one target
        if (p->session().is_closed())
            close_p = p;

        Z_GDU *gdu = p->response().get();
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_searchResponse)
        {
            Z_APDU *b_apdu = gdu->u.z3950;
            Z_SearchResponse *b_resp = b_apdu->u.searchResponse;

            record_diagnostics(b_resp->records, z_diag, odr, no_successful);

            BackendSet backendSet;
            backendSet.m_backend = *bit;
            backendSet.m_count = *b_resp->resultCount;
            result_set_size += *b_resp->resultCount;
            resultSet.m_backend_sets.push_back(backendSet);
        }
    }

    Z_APDU *f_apdu = odr.create_searchResponse(apdu_req, 0, 0);
    Z_SearchResponse *f_resp = f_apdu->u.searchResponse;

    yaz_log(YLOG_DEBUG, "no_successful=%d is_closed=%s hide_errors=%s",
            no_successful,
            close_p ? "true" : "false",
            m_p->m_hide_errors ? "true" : "false");
    *f_resp->resultCount = result_set_size;
    if (close_p && (no_successful == 0 || !m_p->m_hide_unavailable))
    {
        package.session().close();
        package.response() = close_p->response();
        return;
    }
    if (z_diag && (no_successful == 0 || !m_p->m_hide_errors))
    {
        f_resp->records = (Z_Records *)
            odr_malloc(odr, sizeof(*f_resp->records));
        if (z_diag->num_diagRecs > 1)
        {
            f_resp->records->which = Z_Records_multipleNSD;
            f_resp->records->u.multipleNonSurDiagnostics = z_diag;
        }
        else
        {
            f_resp->records->which = Z_Records_NSD;
            f_resp->records->u.nonSurrogateDiagnostic =
                z_diag->diagRecs[0]->u.defaultFormat;
        }
    }
    // assume OK
    m_sets[resultSet.m_setname] = resultSet;

    Odr_int number;
    mp::util::piggyback(*req->smallSetUpperBound,
                        *req->largeSetLowerBound,
                        *req->mediumSetPresentNumber,
                        0, 0,
                        result_set_size,
                        number, 0);
    Package pp(package.session(), package.origin());
    if (z_diag == 0 && number > 0)
    {
        pp.copy_filter(package);
        Z_APDU *p_apdu = zget_APDU(odr, Z_APDU_presentRequest);
        Z_PresentRequest *p_req = p_apdu->u.presentRequest;
        p_req->preferredRecordSyntax = req->preferredRecordSyntax;
        p_req->resultSetId = req->resultSetName;
        *p_req->resultSetStartPoint = 1;
        *p_req->numberOfRecordsRequested = number;
        pp.request() = p_apdu;
        present(pp, p_apdu);

        if (pp.session().is_closed())
            package.session().close();

        Z_GDU *gdu = pp.response().get();
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_presentResponse)
        {
            Z_PresentResponse *p_res = gdu->u.z3950->u.presentResponse;
            f_resp->records = p_res->records;
            *f_resp->numberOfRecordsReturned =
                *p_res->numberOfRecordsReturned;
            *f_resp->nextResultSetPosition =
                *p_res->nextResultSetPosition;
        }
        else
        {
            package.response() = pp.response();
            return;
        }
    }
    package.response() = f_apdu; // in this scope because of p
}

void yf::Multi::Frontend::present(mp::Package &package, Z_APDU *apdu_req)
{
    // create present request
    Z_PresentRequest *req = apdu_req->u.presentRequest;

    Sets_it it;
    it = m_sets.find(std::string(req->resultSetId));
    if (it == m_sets.end())
    {
        mp::odr odr;
        Z_APDU *apdu =
            odr.create_presentResponse(
                apdu_req,
                YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
                req->resultSetId);
        package.response() = apdu;
        return;
    }
    std::list<Multi::FrontendSet::PresentJob> jobs;
    int start = *req->resultSetStartPoint;
    int number = *req->numberOfRecordsRequested;

    if (m_p->m_merge_type == round_robin)
        it->second.round_robin(start, number, jobs);
    else if (m_p->m_merge_type == serve_order)
        it->second.serve_order(start, number, jobs);

    if (0)
    {
        std::list<Multi::FrontendSet::PresentJob>::const_iterator jit;
        for (jit = jobs.begin(); jit != jobs.end(); jit++)
        {
            yaz_log(YLOG_DEBUG, "job pos=%d", jit->m_pos);
        }
    }

    std::list<BackendPtr> present_backend_list;

    std::list<BackendSet>::const_iterator bsit;
    bsit = it->second.m_backend_sets.begin();
    for (; bsit != it->second.m_backend_sets.end(); bsit++)
    {
        int start = -1;
        int end = -1;
        {
            std::list<Multi::FrontendSet::PresentJob>::const_iterator jit;
            for (jit = jobs.begin(); jit != jobs.end(); jit++)
            {
                if (jit->m_backend == bsit->m_backend)
                {
                    if (start == -1 || jit->m_pos < start)
                        start = jit->m_pos;
                    if (end == -1 || jit->m_pos > end)
                        end = jit->m_pos;
                }
            }
        }
        if (start != -1)
        {
            std::list<Multi::FrontendSet::PresentJob>::iterator jit;
            for (jit = jobs.begin(); jit != jobs.end(); jit++)
            {
                if (jit->m_backend == bsit->m_backend)
                {
                    if (jit->m_pos >= start && jit->m_pos <= end)
                        jit->m_start = start;
                }
            }

            PackagePtr p = bsit->m_backend->m_package;

            *req->resultSetStartPoint = start;
            *req->numberOfRecordsRequested = end - start + 1;

            p->request() = apdu_req;
            p->copy_filter(package);

            present_backend_list.push_back(bsit->m_backend);
        }
    }
    multi_move(present_backend_list);

    // look at each response
    Z_DiagRecs *z_diag = 0;
    int no_successful = 0;
    mp::odr odr;
    PackagePtr close_p;

    std::list<BackendPtr>::const_iterator pbit = present_backend_list.begin();
    for (; pbit != present_backend_list.end(); pbit++)
    {
        PackagePtr p = (*pbit)->m_package;

        // save closing package for at least one target
        if (p->session().is_closed())
            close_p = p;

        Z_GDU *gdu = p->response().get();
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_presentResponse)
        {
            Z_APDU *b_apdu = gdu->u.z3950;
            Z_PresentResponse *b_resp = b_apdu->u.presentResponse;

            record_diagnostics(b_resp->records, z_diag, odr, no_successful);
        }
    }

    Z_APDU *f_apdu = odr.create_presentResponse(apdu_req, 0, 0);
    Z_PresentResponse *f_resp = f_apdu->u.presentResponse;

    if (close_p && (no_successful == 0 || !m_p->m_hide_errors))
    {
        package.session().close();
        package.response() = close_p->response();
        return;
    }
    if (z_diag && (no_successful == 0 || !m_p->m_hide_errors))
    {
        f_resp->records = (Z_Records *)
            odr_malloc(odr, sizeof(*f_resp->records));
        if (z_diag->num_diagRecs > 1)
        {
            f_resp->records->which = Z_Records_multipleNSD;
            f_resp->records->u.multipleNonSurDiagnostics = z_diag;
        }
        else
        {
            f_resp->records->which = Z_Records_NSD;
            f_resp->records->u.nonSurrogateDiagnostic =
                z_diag->diagRecs[0]->u.defaultFormat;
        }
    }
    else if (number < 0 || (size_t) number > jobs.size())
    {
        f_apdu =
            odr.create_presentResponse(
                apdu_req,
                YAZ_BIB1_PRESENT_REQUEST_OUT_OF_RANGE,
                0);
    }
    else
    {
        f_resp->records = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
        Z_Records * records = f_resp->records;
        records->which = Z_Records_DBOSD;
        records->u.databaseOrSurDiagnostics =
            (Z_NamePlusRecordList *)
            odr_malloc(odr, sizeof(Z_NamePlusRecordList));
        Z_NamePlusRecordList *nprl = records->u.databaseOrSurDiagnostics;
        nprl->num_records = jobs.size();
        nprl->records = (Z_NamePlusRecord**)
            odr_malloc(odr, sizeof(Z_NamePlusRecord *) * nprl->num_records);
        int i = 0;
        std::list<Multi::FrontendSet::PresentJob>::const_iterator jit;
        for (jit = jobs.begin(); jit != jobs.end(); jit++)
        {
            PackagePtr p = jit->m_backend->m_package;

            Z_GDU *gdu = p->response().get();
            Z_APDU *b_apdu = gdu->u.z3950;
            int inside_pos = jit->m_pos - jit->m_start;
            Z_Records *records = b_apdu->u.presentResponse->records;

            if (records && records->which == Z_Records_DBOSD
                && inside_pos < 
                records->u.databaseOrSurDiagnostics->num_records)
            {
                nprl->records[i] = (Z_NamePlusRecord*)
                    odr_malloc(odr, sizeof(Z_NamePlusRecord));
                *nprl->records[i] = *records->
                    u.databaseOrSurDiagnostics->records[inside_pos];
                nprl->records[i]->databaseName =
                    odr_strdup(odr, jit->m_backend->m_vhost.c_str());
                i++;
            }
        }
        nprl->num_records = i; // usually same as jobs.size();
        *f_resp->nextResultSetPosition = start + i;
        *f_resp->numberOfRecordsReturned = i;
    }
    package.response() = f_apdu;
}

bool yf::Multi::ScanTermInfo::operator < (const ScanTermInfo &k) const
{
    return m_norm_term < k.m_norm_term;
}

bool yf::Multi::ScanTermInfo::operator == (const ScanTermInfo &k) const
{
    return m_norm_term == k.m_norm_term;
}

Z_Entry *yf::Multi::ScanTermInfo::get_entry(ODR odr)
{
    Z_Entry *e = (Z_Entry *)odr_malloc(odr, sizeof(*e));
    e->which = Z_Entry_termInfo;
    Z_TermInfo *t;
    t = e->u.termInfo = (Z_TermInfo *) odr_malloc(odr, sizeof(*t));
    t->suggestedAttributes = 0;
    t->displayTerm = 0;
    t->alternativeTerm = 0;
    t->byAttributes = 0;
    t->otherTermInfo = 0;
    t->globalOccurrences = odr_intdup(odr, m_count);
    t->term = (Z_Term *) odr_malloc(odr, sizeof(*t->term));
    t->term->which = Z_Term_general;
    t->term->u.general = odr_create_Odr_oct(odr,
                                 m_norm_term.c_str(), m_norm_term.size());
    return e;
}

void yf::Multi::Frontend::relay_apdu(mp::Package &package, Z_APDU *apdu_req)
{
    std::list<BackendPtr>::const_iterator bit;
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        PackagePtr p = (*bit)->m_package;
        mp::odr odr;

        p->request() = apdu_req;
        p->copy_filter(package);
    }
    multi_move(m_backend_list);
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        PackagePtr p = (*bit)->m_package;

        if (p->session().is_closed()) // if any backend closes, close frontend
            package.session().close();

        package.response() = p->response();
    }
}


void yf::Multi::Frontend::scan(mp::Package &package, Z_APDU *apdu_req)
{
    Z_ScanRequest *req = apdu_req->u.scanRequest;

    std::list<BackendPtr>::const_iterator bit;
    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        PackagePtr p = (*bit)->m_package;
        yazpp_1::GDU gdu1(apdu_req);
        mp::odr odr;
        Z_ScanRequest *req1 = gdu1.get()->u.z3950->u.scanRequest;

        if (!mp::util::set_databases_from_zurl(odr, (*bit)->m_vhost,
                                               &req1->num_databaseNames,
                                               &req1->databaseNames))
        {
            req1->num_databaseNames = req->num_databaseNames;
            req1->databaseNames = req->databaseNames;
        }
        p->request() = gdu1;
        p->copy_filter(package);
    }
    multi_move(m_backend_list);

    ScanTermInfoList entries_before;
    ScanTermInfoList entries_after;
    int no_before = 0;
    int no_after = 0;

    for (bit = m_backend_list.begin(); bit != m_backend_list.end(); bit++)
    {
        PackagePtr p = (*bit)->m_package;

        if (p->session().is_closed()) // if any backend closes, close frontend
            package.session().close();

        Z_GDU *gdu = p->response().get();
        if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
            Z_APDU_scanResponse)
        {
            Z_ScanResponse *res = gdu->u.z3950->u.scanResponse;

            if (res->entries && res->entries->nonsurrogateDiagnostics)
            {
                // failure
                mp::odr odr;
                Z_APDU *f_apdu = odr.create_scanResponse(apdu_req, 1, 0);
                Z_ScanResponse *f_res = f_apdu->u.scanResponse;

                f_res->entries->nonsurrogateDiagnostics =
                    res->entries->nonsurrogateDiagnostics;
                f_res->entries->num_nonsurrogateDiagnostics =
                    res->entries->num_nonsurrogateDiagnostics;

                package.response() = f_apdu;
                return;
            }

            if (res->entries && res->entries->entries)
            {
                Z_Entry **entries = res->entries->entries;
                int num_entries = res->entries->num_entries;
                int position = 1;
                if (req->preferredPositionInResponse)
                    position = *req->preferredPositionInResponse;
                if (res->positionOfTerm)
                    position = *res->positionOfTerm;

                // before
                int i;
                for (i = 0; i<position-1 && i<num_entries; i++)
                {
                    Z_Entry *ent = entries[i];

                    if (ent->which == Z_Entry_termInfo)
                    {
                        ScanTermInfo my;

                        Odr_int *occur = ent->u.termInfo->globalOccurrences;
                        my.m_count = occur ? *occur : 0;

                        if (ent->u.termInfo->term->which == Z_Term_general)
                        {
                            my.m_norm_term = std::string(
                                (const char *)
                                ent->u.termInfo->term->u.general->buf,
                                ent->u.termInfo->term->u.general->len);
                        }
                        if (my.m_norm_term.length())
                        {
                            ScanTermInfoList::iterator it =
                                entries_before.begin();
                            while (it != entries_before.end() && my <*it)
                                it++;
                            if (it != entries_before.end() && my == *it)
                            {
                                it->m_count += my.m_count;
                            }
                            else
                            {
                                entries_before.insert(it, my);
                                no_before++;
                            }
                        }
                    }
                }
                // after
                if (position <= 0)
                    i = 0;
                else
                    i = position-1;
                for ( ; i<num_entries; i++)
                {
                    Z_Entry *ent = entries[i];

                    if (ent->which == Z_Entry_termInfo)
                    {
                        ScanTermInfo my;

                        Odr_int *occur = ent->u.termInfo->globalOccurrences;
                        my.m_count = occur ? *occur : 0;

                        if (ent->u.termInfo->term->which == Z_Term_general)
                        {
                            my.m_norm_term = std::string(
                                (const char *)
                                ent->u.termInfo->term->u.general->buf,
                                ent->u.termInfo->term->u.general->len);
                        }
                        if (my.m_norm_term.length())
                        {
                            ScanTermInfoList::iterator it =
                                entries_after.begin();
                            while (it != entries_after.end() && *it < my)
                                it++;
                            if (it != entries_after.end() && my == *it)
                            {
                                it->m_count += my.m_count;
                            }
                            else
                            {
                                entries_after.insert(it, my);
                                no_after++;
                            }
                        }
                    }
                }

            }
        }
        else
        {
            // if any target does not return scan response - return that
            package.response() = p->response();
            return;
        }
    }

    if (false)
    {
        std::cout << "BEFORE\n";
        ScanTermInfoList::iterator it = entries_before.begin();
        for(; it != entries_before.end(); it++)
        {
            std::cout << " " << it->m_norm_term << " " << it->m_count << "\n";
        }

        std::cout << "AFTER\n";
        it = entries_after.begin();
        for(; it != entries_after.end(); it++)
        {
            std::cout << " " << it->m_norm_term << " " << it->m_count << "\n";
        }
    }

    if (false)
    {
        mp::odr odr;
        Z_APDU *f_apdu = odr.create_scanResponse(apdu_req, 1, "not implemented");
        package.response() = f_apdu;
    }
    else
    {
        mp::odr odr;
        Z_APDU *f_apdu = odr.create_scanResponse(apdu_req, 0, 0);
        Z_ScanResponse *resp = f_apdu->u.scanResponse;

        int number_returned = *req->numberOfTermsRequested;
        int position_returned = *req->preferredPositionInResponse;

        resp->entries->num_entries = number_returned;
        resp->entries->entries = (Z_Entry**)
            odr_malloc(odr, sizeof(Z_Entry*) * number_returned);
        int i;

        int lbefore = entries_before.size();
        if (lbefore < position_returned-1)
            position_returned = lbefore+1;

        ScanTermInfoList::iterator it = entries_before.begin();
        for (i = 0; i<position_returned-1 && it != entries_before.end(); i++, it++)
        {
            resp->entries->entries[position_returned-2-i] = it->get_entry(odr);
        }

        it = entries_after.begin();

        if (position_returned <= 0)
            i = 0;
        else
            i = position_returned-1;
        for (; i<number_returned && it != entries_after.end(); i++, it++)
        {
            resp->entries->entries[i] = it->get_entry(odr);
        }

        number_returned = i;

        resp->positionOfTerm = odr_intdup(odr, position_returned);
        resp->numberOfEntriesReturned = odr_intdup(odr, number_returned);
        resp->entries->num_entries = number_returned;

        package.response() = f_apdu;
    }
}


void yf::Multi::process(mp::Package &package) const
{
    FrontendPtr f = m_p->get_frontend(package);

    Z_GDU *gdu = package.request().get();

    if (gdu && gdu->which == Z_GDU_Z3950 && gdu->u.z3950->which ==
        Z_APDU_initRequest && !f->m_is_multi)
    {
        f->init(package, gdu);
    }
    else if (!f->m_is_multi)
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
        else if (apdu->which == Z_APDU_close)
        {
            f->relay_apdu(package, apdu);
        }
        else
        {
            mp::odr odr;

            package.response() = odr.create_close(
                apdu, Z_Close_protocolError,
                "unsupported APDU in filter multi");

            package.session().close();
        }
    }
    m_p->release_frontend(package);
}

void mp::filter::Multi::configure(const xmlNode * ptr, bool test_only,
                                  const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "target"))
        {
            std::string auth;
            std::string route = mp::xml::get_route(ptr, auth);
            std::string target = mp::xml::get_text(ptr);
            if (target.length() == 0)
                target = route;
            m_p->m_route_patterns.push_back(Multi::Map(target, route, auth));
        }
        else if (!strcmp((const char *) ptr->name, "hideunavailable"))
        {
            m_p->m_hide_unavailable = true;
        }
        else if (!strcmp((const char *) ptr->name, "hideerrors"))
        {
            m_p->m_hide_errors = true;
        }
        else if (!strcmp((const char *) ptr->name, "mergetype"))
        {
            std::string mergetype = mp::xml::get_text(ptr);
            if (mergetype == "roundrobin")
                m_p->m_merge_type = round_robin;
            else if (mergetype == "serveorder")
                m_p->m_merge_type = serve_order;
            else
                throw mp::filter::FilterException
                    ("Bad mergetype "  + mergetype + " in multi filter");

        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element "
                 + std::string((const char *) ptr->name)
                 + " in multi filter");
        }
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::Multi;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_multi = {
        0,
        "multi",
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

