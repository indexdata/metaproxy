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

#include "filter_z3950_client.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>

#include <map>
#include <stdexcept>
#include <list>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/xtime.hpp>

#include <yaz/zgdu.h>
#include <yaz/log.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>
#include <yaz/oid_db.h>
#include <yaz/charneg.h>

#include <yazpp/socket-manager.h>
#include <yazpp/pdu-assoc.h>
#include <yazpp/z-assoc.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Z3950Client::Assoc : public yazpp_1::Z_Assoc{
            friend class Rep;
            Assoc(yazpp_1::SocketManager *socket_manager,
                  yazpp_1::IPDU_Observable *PDU_Observable,
                  std::string host, int connect_timeout,
                  int init_timeout, int max_sockets_timeout,
                  int general_timeout);
            ~Assoc();
            void connectNotify();
            void failNotify();
            void timeoutNotify();
            void recv_GDU(Z_GDU *gdu, int len);
            void fixup_nsd(ODR odr, Z_Records *records);
            void fixup_nsd(ODR odr, Z_DefaultDiagFormat *nsd);
            void fixup_init(ODR odr, Z_InitResponse *initrs);
            yazpp_1::IPDU_Observer* sessionNotify(
                yazpp_1::IPDU_Observable *the_PDU_Observable,
                int fd);

            yazpp_1::SocketManager *m_socket_manager;
            yazpp_1::IPDU_Observable *m_PDU_Observable;
            Package *m_package;
            bool m_in_use;
            bool m_waiting;
            bool m_destroyed;
            int m_connected; // 0=not connected, 1=init phase, 2=rest
            bool m_has_closed;
            int m_queue_len;
            int m_time_elapsed;
            int m_connect_time_max;
            int m_init_time_max;
            int m_max_sockets_timeout_sec;
            int m_general_time_max;
            std::string m_host;
        };

        class Z3950Client::Rep {
        public:
            // number of seconds to wait before we give up request
            int m_general_timeout_sec;
            int m_connect_timeout_sec;
            int m_init_timeout_sec;
            int m_max_sockets_timeout_sec;
            int m_max_sockets;
            bool m_force_close;
            bool m_client_ip;
            bool m_bind_host;
            std::string m_charset;
            std::string m_default_target;
            std::string m_force_target;
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<mp::Session,Z3950Client::Assoc *> m_clients;
            Z3950Client::Assoc *get_assoc(Package &package);
            void send_and_receive(Package &package,
                                  yf::Z3950Client::Assoc *c);
            void release_assoc(Package &package);
        };
    }
}

using namespace mp;

yf::Z3950Client::Assoc::Assoc(yazpp_1::SocketManager *socket_manager,
                              yazpp_1::IPDU_Observable *PDU_Observable,
                              std::string host,
                              int connect_timeout,
                              int init_timeout, int max_sockets_timeout,
                              int general_timeout)
    :  Z_Assoc(PDU_Observable),
       m_socket_manager(socket_manager), m_PDU_Observable(PDU_Observable),
       m_package(0), m_in_use(true), m_waiting(false),
       m_destroyed(false), m_connected(0), m_has_closed(false),
       m_queue_len(1),
       m_time_elapsed(0),
       m_connect_time_max(connect_timeout),
       m_init_time_max(init_timeout),
       m_max_sockets_timeout_sec(max_sockets_timeout),
       m_general_time_max(general_timeout),
       m_host(host)
{
}

yf::Z3950Client::Assoc::~Assoc()
{
}

void yf::Z3950Client::Assoc::connectNotify()
{
    m_waiting = false;

    m_connected = 1;
}

void yf::Z3950Client::Assoc::failNotify()
{
    m_waiting = false;

    mp::odr odr;

    if (m_package)
    {
        Z_GDU *gdu = m_package->request().get();
        Z_APDU *apdu = 0;
        if (gdu && gdu->which == Z_GDU_Z3950)
            apdu = gdu->u.z3950;

        m_package->response() = odr.create_close(apdu, Z_Close_peerAbort, 0);
        m_package->session().close();
    }
}

void yf::Z3950Client::Assoc::timeoutNotify()
{
    m_time_elapsed++;
    if ((m_connected == 0 && m_time_elapsed >= m_connect_time_max)
        || (m_connected == 1 && m_time_elapsed >= m_init_time_max)
        || (m_connected >= 2 && m_time_elapsed >= m_general_time_max))
    {
        m_waiting = false;

        mp::odr odr;

        if (m_package)
        {
            Z_GDU *gdu = m_package->request().get();
            Z_APDU *apdu = 0;
            if (gdu && gdu->which == Z_GDU_Z3950)
                apdu = gdu->u.z3950;

            if (m_connected == 2)
                m_package->response() =
                    odr.create_close(apdu, Z_Close_lackOfActivity, 0);
            else
                m_package->response() =
                    odr.create_close(apdu, Z_Close_peerAbort, 0);

            m_package->session().close();
        }
    }
}

void yf::Z3950Client::Assoc::fixup_nsd(ODR odr, Z_DefaultDiagFormat *nsd)
{
    std::string addinfo;

    // should really check for nsd->which.. But union has two members
    // containing almost same data
    const char *v2Addinfo = nsd->u.v2Addinfo;
    //  Z_InternationalString *v3Addinfo;
    if (v2Addinfo && *v2Addinfo)
    {
        addinfo.assign(nsd->u.v2Addinfo);
        addinfo += " ";
    }
    addinfo += "(backend=" + m_host + ")";
    nsd->u.v2Addinfo = odr_strdup(odr, addinfo.c_str());
}

void yf::Z3950Client::Assoc::fixup_nsd(ODR odr, Z_Records *records)
{
    if (records && records->which == Z_Records_NSD)
    {
        fixup_nsd(odr, records->u.nonSurrogateDiagnostic);
    }
    if (records && records->which == Z_Records_multipleNSD)
    {
        Z_DiagRecs *drecs = records->u.multipleNonSurDiagnostics;
        int i;
        for (i = 0; i < drecs->num_diagRecs; i++)
        {
            Z_DiagRec *dr = drecs->diagRecs[i];

            if (dr->which == Z_DiagRec_defaultFormat)
                fixup_nsd(odr, dr->u.defaultFormat);
        }
    }
}

void yf::Z3950Client::Assoc::fixup_init(ODR odr, Z_InitResponse *initrs)
{
    Z_External *uif = initrs->userInformationField;

    if (uif && uif->which == Z_External_userInfo1)
    {
        Z_OtherInformation *ui = uif->u.userInfo1;
        int i;
        for (i = 0; i < ui->num_elements; i++)
        {
            Z_OtherInformationUnit *unit = ui->list[i];
            if (unit->which == Z_OtherInfo_externallyDefinedInfo &&
                unit->information.externallyDefinedInfo &&
                unit->information.externallyDefinedInfo->which ==
                Z_External_diag1)
            {
                Z_DiagnosticFormat *diag =
                    unit->information.externallyDefinedInfo->u.diag1;
                int j;
                for (j = 0; j < diag->num; j++)
                {
                    Z_DiagnosticFormat_s *ds = diag->elements[j];
                    if (ds->which == Z_DiagnosticFormat_s_defaultDiagRec)
                    {
                        Z_DefaultDiagFormat *r = ds->u.defaultDiagRec;
                        char *oaddinfo = r->u.v2Addinfo;
                        char *naddinfo = (char *) odr_malloc(
                            odr,
                            (oaddinfo ? strlen(oaddinfo) : 0) + 20 +
                            m_host.length());
                        *naddinfo = '\0';
                        if (oaddinfo && *oaddinfo)
                        {
                            strcat(naddinfo, oaddinfo);
                            strcat(naddinfo, " ");
                        }
                        strcat(naddinfo, "(backend=");
                        strcat(naddinfo, m_host.c_str());
                        strcat(naddinfo, ")");

                        r->u.v2Addinfo = naddinfo;
                    }
                }
            }
        }
    }
}

void yf::Z3950Client::Assoc::recv_GDU(Z_GDU *gdu, int len)
{
    m_waiting = false;

    if (m_package)
    {
        mp::odr odr; // must be in scope for response() = assignment
        if (gdu && gdu->which == Z_GDU_Z3950)
        {
            Z_APDU *apdu = gdu->u.z3950;
            switch (apdu->which)
            {
            case Z_APDU_searchResponse:
                fixup_nsd(odr, apdu->u.searchResponse->records);
                break;
            case Z_APDU_presentResponse:
                fixup_nsd(odr, apdu->u.presentResponse->records);
                break;
            case Z_APDU_initResponse:
                fixup_init(odr, apdu->u.initResponse);
                m_connected = 2;
                break;
            }
        }
        m_package->response() = gdu;
    }
}

yazpp_1::IPDU_Observer *yf::Z3950Client::Assoc::sessionNotify(
    yazpp_1::IPDU_Observable *the_PDU_Observable,
    int fd)
{
    return 0;
}


yf::Z3950Client::Z3950Client() :  m_p(new yf::Z3950Client::Rep)
{
    m_p->m_connect_timeout_sec = 10;
    m_p->m_init_timeout_sec = 10;
    m_p->m_general_timeout_sec = 30;
    m_p->m_max_sockets = 0;
    m_p->m_max_sockets_timeout_sec = 15;
    m_p->m_force_close = false;
    m_p->m_client_ip = false;
    m_p->m_bind_host = false;
}

yf::Z3950Client::~Z3950Client() {
}

yf::Z3950Client::Assoc *yf::Z3950Client::Rep::get_assoc(Package &package)
{
    // only one thread messes with the clients list at a time
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<mp::Session,yf::Z3950Client::Assoc *>::iterator it;

    Z_GDU *gdu = package.request().get();

    int max_sockets = package.origin().get_max_sockets();
    if (max_sockets == 0)
        max_sockets = m_max_sockets;

    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        it->second->m_queue_len++;
        while (true)
        {
#if 0
            // double init .. NOT working yet
            if (gdu && gdu->which == Z_GDU_Z3950 &&
                gdu->u.z3950->which == Z_APDU_initRequest)
            {
                yazpp_1::SocketManager *s = it->second->m_socket_manager;
                delete it->second;  // destroy Z_Assoc
                delete s;    // then manager
                m_clients.erase(it);
                break;
            }
#endif
            if (!it->second->m_in_use)
            {
                it->second->m_in_use = true;
                return it->second;
            }
            m_cond_session_ready.wait(lock);
        }
    }
    if (!gdu || gdu->which != Z_GDU_Z3950)
    {
        package.move();
        return 0;
    }
    // new Z39.50 session ..
    Z_APDU *apdu = gdu->u.z3950;
    // check that it is init. If not, close
    if (apdu->which != Z_APDU_initRequest)
    {
        mp::odr odr;

        package.response() = odr.create_close(apdu,
                                              Z_Close_protocolError,
                                              "First PDU was not an "
                                              "Initialize Request");
        package.session().close();
        return 0;
    }
    std::string target = m_force_target;
    if (!target.length())
    {
        target = m_default_target;
        std::list<std::string> vhosts;
        mp::util::remove_vhost_otherinfo(&apdu->u.initRequest->otherInfo,
                                             vhosts);
        size_t no_vhosts = vhosts.size();
        if (no_vhosts == 1)
        {
            std::list<std::string>::const_iterator v_it = vhosts.begin();
            target = *v_it;
        }
        else if (no_vhosts == 0)
        {
            if (!target.length())
            {
                // no default target. So we don't know where to connect
                mp::odr odr;
                package.response() = odr.create_initResponse(
                    apdu,
                    YAZ_BIB1_INIT_NEGOTIATION_OPTION_REQUIRED,
                    "z3950_client: No vhost given");

                package.session().close();
                return 0;
            }
        }
        else if (no_vhosts > 1)
        {
            mp::odr odr;
            package.response() = odr.create_initResponse(
                apdu,
                YAZ_BIB1_COMBI_OF_SPECIFIED_DATABASES_UNSUPP,
                "z3950_client: Can not cope with multiple vhosts");
            package.session().close();
            return 0;
        }
    }

    // see if we have reached max number of clients (max-sockets)
    boost::xtime xt;
    xtime_get(&xt,
#if BOOST_VERSION >= 105000
              boost::TIME_UTC_
#else
              boost::TIME_UTC
#endif
        );
    xt.sec += m_max_sockets_timeout_sec;
    while (max_sockets)
    {
        int no_not_in_use = 0;
        int number = 0;
        it = m_clients.begin();
        for (; it != m_clients.end(); it++)
        {
            yf::Z3950Client::Assoc *as = it->second;
            if (!strcmp(as->m_host.c_str(), target.c_str()))
            {
                number++;
                if (!as->m_in_use)
                    no_not_in_use++;
            }
        }
        yaz_log(YLOG_LOG, "Found num=%d/max=%d/idle=%d connections for %s",
                number, max_sockets, no_not_in_use, target.c_str());
        if (number < max_sockets)
            break;
        if (no_not_in_use == 0) // all in use..
        {
            mp::odr odr;

            package.response() = odr.create_initResponse(
                apdu, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                "z3950_client: max sessions");
            package.session().close();
            return 0;
        }
        if (!m_cond_session_ready.timed_wait(lock, xt))
        {
            mp::odr odr;

            package.response() = odr.create_initResponse(
                apdu, YAZ_BIB1_TEMPORARY_SYSTEM_ERROR,
                "z3950_client: max sessions");
            package.session().close();
            return 0;
        }
    }

    yazpp_1::SocketManager *sm = new yazpp_1::SocketManager;
    yazpp_1::PDU_Assoc *pdu_as = new yazpp_1::PDU_Assoc(sm);
    yf::Z3950Client::Assoc *as =
        new yf::Z3950Client::Assoc(sm, pdu_as, target.c_str(),
                                   m_connect_timeout_sec,
                                   m_init_timeout_sec,
                                   m_max_sockets_timeout_sec,
                                   m_general_timeout_sec);
    m_clients[package.session()] = as;
    return as;
}

static void set_charset_proposal(ODR odr, Z_InitRequest *req, const char *charset)
{
    Z_OtherInformation **p = &req->otherInfo;
    Z_OtherInformationUnit *oi;

    if (*p)
    {
        int i;
        for (i = 0; i < (*p)->num_elements; i++)
        {
            Z_External *ext = (*p)->list[i]->information.externallyDefinedInfo;
            if ((*p)->list[i]->which == Z_OtherInfo_externallyDefinedInfo
                && ext &&
                ext->which == Z_External_charSetandLanguageNegotiation)
                return;
        }
    }
    if ((oi = yaz_oi_update(p, odr, 0, 0, 0)))
    {
        ODR_MASK_SET(req->options, Z_Options_negotiationModel);
        oi->which = Z_OtherInfo_externallyDefinedInfo;
        oi->information.externallyDefinedInfo =
            yaz_set_proposal_charneg_list(odr, ",",
                                          charset,
                                          0 /* lang */,
                                          1 /* records included */);
    }
}

void yf::Z3950Client::Rep::send_and_receive(Package &package,
                                            yf::Z3950Client::Assoc *c)
{
    if (c->m_destroyed)
        return;

    c->m_package = &package;

    if (package.session().is_closed() && c->m_connected && !c->m_has_closed
        && m_force_close)
    {
        mp::odr odr;

        package.request() = odr.create_close(
            0, Z_Close_finished, "z3950_client");
        c->m_package = 0; // don't inspect response
    }
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950)
        return;

    if (gdu->u.z3950->which == Z_APDU_close)
        c->m_has_closed = true;

    Z_APDU *apdu = gdu->u.z3950;

    // prepare connect
    c->m_time_elapsed = 0;
    c->m_waiting = true;
    if (!c->m_connected)
    {
        std::string host(c->m_host);

        if (m_bind_host)
        {
            std::string bind_host = package.origin().get_bind_address();
            if (bind_host.length())
            {
                host.append(" ");
                host.append(bind_host);
            }
        }
        if (c->client(host.c_str()))
        {
            mp::odr odr;
            package.response() =
                odr.create_close(gdu->u.z3950, Z_Close_peerAbort, 0);
            package.session().close();
            return;
        }
        c->timeout(1);  // so timeoutNotify gets called once per second


        while (!c->m_destroyed && c->m_waiting
               && c->m_socket_manager->processEvent() > 0)
            ;
    }
    if (!c->m_connected)
    {
        return;
    }
    mp::odr odr;
    if (m_client_ip)
    {
        std::string peer_name2 = package.origin().get_address();
        if (apdu->which == Z_APDU_initRequest && peer_name2.length())
        {
            Z_OtherInformation **oi = &apdu->u.initRequest->otherInfo;
            char *peer_name1 =
                yaz_oi_get_string_oid(oi, yaz_oid_userinfo_client_ip, 1, 1);
            std::string pcomb;
            if (peer_name1)
            {
                pcomb.append(peer_name1);
                pcomb.append(", ");
            }
            pcomb.append(peer_name2);
            yaz_oi_set_string_oid(&apdu->u.initRequest->otherInfo,
                                  odr, yaz_oid_userinfo_client_ip,
                                  1, pcomb.c_str());
        }
    }
    if (apdu->which == Z_APDU_initRequest && m_charset.length() > 0)
        set_charset_proposal(odr, apdu->u.initRequest, m_charset.c_str());

    // prepare response
    c->m_time_elapsed = 0;
    c->m_waiting = true;

    // relay the package  ..
    int len;
    c->send_GDU(gdu, &len);

    switch (gdu->u.z3950->which)
    {
    case Z_APDU_triggerResourceControlRequest:
        // request only..
        break;
    default:
        // for the rest: wait for a response PDU
        while (!c->m_destroyed && c->m_waiting
               && c->m_socket_manager->processEvent() > 0)
            ;
        break;
    }
}

void yf::Z3950Client::Rep::release_assoc(Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<mp::Session,yf::Z3950Client::Assoc *>::iterator it;

    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        it->second->m_in_use = false;
        it->second->m_queue_len--;

        if (package.session().is_closed())
        {
            // destroy hint (send_and_receive)
            it->second->m_destroyed = true;
            if (it->second->m_queue_len == 0)
            {
                yazpp_1::SocketManager *s = it->second->m_socket_manager;
                delete it->second;  // destroy Z_Assoc
                delete s;    // then manager
                m_clients.erase(it);
            }
        }
        m_cond_session_ready.notify_all();
    }
}

void yf::Z3950Client::process(Package &package) const
{
    yf::Z3950Client::Assoc *c = m_p->get_assoc(package);
    if (c)
    {
        m_p->send_and_receive(package, c);
        m_p->release_assoc(package);
    }
}

void yf::Z3950Client::configure(const xmlNode *ptr, bool test_only,
                                const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "timeout"))
        {
            m_p->m_general_timeout_sec = mp::xml::get_int(ptr, 30);
        }
        else if (!strcmp((const char *) ptr->name, "connect-timeout"))
        {
            m_p->m_connect_timeout_sec = mp::xml::get_int(ptr, 10);
        }
        else if (!strcmp((const char *) ptr->name, "init-timeout"))
        {
            m_p->m_init_timeout_sec = mp::xml::get_int(ptr, 10);
        }
        else if (!strcmp((const char *) ptr->name, "max-sockets-timeout"))
        {
            m_p->m_max_sockets_timeout_sec = mp::xml::get_int(ptr, 15);
        }
        else if (!strcmp((const char *) ptr->name, "default_target"))
        {
            m_p->m_default_target = mp::xml::get_text(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "force_target"))
        {
            m_p->m_force_target = mp::xml::get_text(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "max-sockets"))
        {
            m_p->m_max_sockets = mp::xml::get_int(ptr, 0);
        }
        else if (!strcmp((const char *) ptr->name, "force_close"))
        {
            m_p->m_force_close = mp::xml::get_bool(ptr, 0);
        }
        else if (!strcmp((const char *) ptr->name, "client_ip"))
        {
            m_p->m_client_ip = mp::xml::get_bool(ptr, 0);
        }
        else if (!strcmp((const char *) ptr->name, "charset"))
        {
            m_p->m_charset = mp::xml::get_text(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "bind_host"))
        {
            m_p->m_bind_host = mp::xml::get_bool(ptr, 0);
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
    return new mp::filter::Z3950Client;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_z3950_client = {
        0,
        "z3950_client",
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

