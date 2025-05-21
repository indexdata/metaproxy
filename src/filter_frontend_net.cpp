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

#if HAVE_GETRLIMIT
#include <sys/resource.h>
#endif
#include <sstream>
#include <iomanip>
#include <metaproxy/util.hpp>
#include "pipe.hpp"
#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include "thread_pool_observer.hpp"
#include "filter_frontend_net.hpp"
#include <yazpp/z-assoc.h>
#include <yazpp/pdu-assoc.h>
#include <yazpp/socket-manager.h>
#include <yazpp/limit-connect.h>
#include <yaz/timing.h>
#include <yaz/log.h>
#include <yaz/daemon.h>
#include <yaz/malloc_info.h>
#include "gduutil.hpp"
#include <signal.h>
#include <stdlib.h>
#include <iostream>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class FrontendNet::PeerStat {
            friend class FrontendNet;
            PeerStat();
            ~PeerStat();
            size_t get(const std::string &peer);
            size_t add(const std::string &peer);
            size_t remove(const std::string &peer);
            class Item {
                friend class PeerStat;
                std::string m_peer;
                size_t cnt;
                Item *next;
            };
            Item *items;
        };
        class FrontendNet::Port {
            friend class Rep;
            friend class FrontendNet;
            std::string port;
            std::string route;
            std::string cert_fname;
            int max_recv_bytes;
        };
        class FrontendNet::IP_Pattern {
            friend class Rep;
            friend class FrontendNet;
            std::string pattern;
            int verbose;
            int value;
        };
        class FrontendNet::Rep {
            friend class FrontendNet;

            int m_no_threads;
            int m_max_threads;
            int m_stack_size;
            std::vector<Port> m_ports;
            int m_listen_duration;
            std::list<IP_Pattern> session_timeout;
            std::list<IP_Pattern> connect_max;
            std::list<IP_Pattern> connect_total;
            std::list<IP_Pattern> http_req_max;
            std::string m_msg_config;
            std::string m_stat_req;
            yazpp_1::SocketManager mySocketManager;
            ZAssocServer **az;
            yazpp_1::PDU_Assoc **pdu;
            int m_duration_freq[22];
            double m_duration_lim[22];
            double m_duration_max;
            double m_duration_min;
            double m_duration_total;
            int m_stop_signo;
            PeerStat m_peerStat;
        public:
            Rep();
            ~Rep();
        };
        class FrontendNet::My_Timer_Thread : public yazpp_1::ISocketObserver {
        private:
            yazpp_1::ISocketObservable *m_obs;
            Pipe m_pipe;
            bool m_timeout;
        public:
            My_Timer_Thread(yazpp_1::ISocketObservable *obs, int duration);
            void socketNotify(int event);
            bool timeout();
        };
        class FrontendNet::ZAssocChild : public yazpp_1::Z_Assoc {
        public:
            ~ZAssocChild();
            ZAssocChild(yazpp_1::IPDU_Observable *the_PDU_Observable,
                        mp::ThreadPoolSocketObserver *m_thread_pool_observer,
                        const mp::Package *package,
                        Port *port,
                        Rep *rep,
                        yazpp_1::LimitConnect &limit,
                        const char *peername);
            int m_no_requests;
            Port *m_port;
        private:
            yazpp_1::IPDU_Observer* sessionNotify(
                yazpp_1::IPDU_Observable *the_PDU_Observable,
                int fd);
            void recv_GDU(Z_GDU *apdu, int len);
            void report(Z_HTTP_Request *hreq);
            void failNotify();
            void timeoutNotify();
            void connectNotify();
        private:
            mp::ThreadPoolSocketObserver *m_thread_pool_observer;
            mp::Session m_session;
            mp::Origin m_origin;
            bool m_delete_flag;
            const mp::Package *m_package;
            Rep *m_p;
            yazpp_1::LimitConnect &m_limit_http_req;
            std::string m_peer;
        };
        class FrontendNet::ThreadPoolPackage : public mp::IThreadPoolMsg {
        public:
            ThreadPoolPackage(mp::Package *package,
                              yf::FrontendNet::ZAssocChild *ses,
                              Rep *rep);
            ~ThreadPoolPackage();
            IThreadPoolMsg *handle();
            void result(const char *t_info);
            bool cleanup(void *info);
        private:
            yaz_timing_t timer;
            ZAssocChild *m_assoc_child;
            mp::Package *m_package;
            Rep *m_p;
        };
        class FrontendNet::ZAssocServer : public yazpp_1::Z_Assoc {
        public:
            ~ZAssocServer();
            ZAssocServer(yazpp_1::IPDU_Observable *PDU_Observable,
                         FrontendNet::Port *port,
                         Rep *rep);
            void set_package(const mp::Package *package);
            void set_thread_pool(ThreadPoolSocketObserver *observer);
        private:
            yazpp_1::IPDU_Observer* sessionNotify(
                yazpp_1::IPDU_Observable *the_PDU_Observable,
                int fd);
            void recv_GDU(Z_GDU *apdu, int len);

            void failNotify();
            void timeoutNotify();
            void connectNotify();
        private:
            mp::ThreadPoolSocketObserver *m_thread_pool_observer;
            const mp::Package *m_package;
            yazpp_1::LimitConnect limit_connect;
            yazpp_1::LimitConnect limit_http_req;
            Port *m_port;
            Rep *m_p;
        };
    }
}

yf::FrontendNet::ThreadPoolPackage::ThreadPoolPackage(mp::Package *package,
                                                      ZAssocChild *ses,
                                                      Rep *rep) :
    m_assoc_child(ses), m_package(package), m_p(rep)
{
    timer = yaz_timing_create();
}

yf::FrontendNet::ThreadPoolPackage::~ThreadPoolPackage()
{
    yaz_timing_destroy(&timer); // timer may be NULL
    delete m_package;
}

bool yf::FrontendNet::ThreadPoolPackage::cleanup(void *info)
{
    mp::Session *ses = (mp::Session *) info;
    if (*ses == m_package->session())
    {
        m_assoc_child->m_no_requests--;
        return true;
    }
    return false;
}

void yf::FrontendNet::ThreadPoolPackage::result(const char *t_info)
{
    m_assoc_child->m_no_requests--;

    yazpp_1::GDU *gdu = &m_package->response();

    if (gdu->get())
    {
        int len;
        m_assoc_child->send_GDU(gdu->get(), &len);

        yaz_timing_stop(timer);
        double duration = yaz_timing_get_real(timer);

        size_t ent = 0;
        while (m_p->m_duration_lim[ent] != 0.0 && duration > m_p->m_duration_lim[ent])
            ent++;
        m_p->m_duration_freq[ent]++;

        m_p->m_duration_total += duration;

        if (m_p->m_duration_max < duration)
            m_p->m_duration_max = duration;

        if (m_p->m_duration_min == 0.0 || m_p->m_duration_min > duration)
            m_p->m_duration_min = duration;

        if (m_p->m_msg_config.length())
        {
            Z_GDU *z_gdu = gdu->get();

            std::ostringstream os;
            os  << m_p->m_msg_config << " "
                << *m_package << " "
                << std::fixed << std::setprecision (6) << duration << " ";

            if (z_gdu)
                os << *z_gdu;
            else
                os << "-";

            yaz_log(YLOG_LOG, "%s %s", os.str().c_str(), t_info);
        }
    }
    else if (!m_package->session().is_closed())
    {
        // no response package and yet the session is still open..
        // means that request is unhandled..
        yazpp_1::GDU *gdu_req = &m_package->request();
        Z_GDU *z_gdu = gdu_req->get();
        if (z_gdu && z_gdu->which == Z_GDU_Z3950)
        {
            // For Z39.50, response with a Close and shutdown
            mp::odr odr;
            int len;
            Z_APDU *apdu_response = odr.create_close(
                z_gdu->u.z3950, Z_Close_systemProblem,
                "unhandled Z39.50 request");

            m_assoc_child->send_Z_PDU(apdu_response, &len);
        }
        else if (z_gdu && z_gdu->which == Z_GDU_HTTP_Request)
        {
            // For HTTP, respond with Server Error
            int len;
            mp::odr odr;
            Z_GDU *zgdu_res
                = odr.create_HTTP_Response(m_package->session(),
                                           z_gdu->u.HTTP_Request, 500);
            m_assoc_child->send_GDU(zgdu_res, &len);
        }
        m_package->session().close();
    }

    if (m_assoc_child->m_no_requests == 0 && m_package->session().is_closed())
    {
        m_assoc_child->close();
    }


    delete this;
}

mp::IThreadPoolMsg *yf::FrontendNet::ThreadPoolPackage::handle()
{
    m_package->move(m_assoc_child->m_port->route);
    return this;
}

yf::FrontendNet::ZAssocChild::ZAssocChild(
    yazpp_1::IPDU_Observable *PDU_Observable,
    mp::ThreadPoolSocketObserver *my_thread_pool,
    const mp::Package *package,
    Port *port, Rep *rep,
    yazpp_1::LimitConnect &limit_http_req,
    const char *peername)
    :  Z_Assoc(PDU_Observable), m_p(rep), m_limit_http_req(limit_http_req)
    , m_peer(peername)
{
    m_thread_pool_observer = my_thread_pool;
    m_no_requests = 0;
    m_delete_flag = false;
    m_package = package;
    m_port = port;
    std::string addr;
    addr.append(peername);
    addr.append(" ");
    addr.append(port->port);
    m_p->m_peerStat.add(m_peer);
    m_origin.set_tcpip_address(addr, m_session.id());
    int session_timeout = 300; // 5 minutes
    if (peername) {
        std::list<IP_Pattern>::const_iterator it = m_p->session_timeout.begin();
        for (; it != m_p->session_timeout.end(); it++)
            if (mp::util::match_ip(it->pattern, peername))
            {
                if (it->verbose > 1)
                    yaz_log(YLOG_LOG, "timeout pattern=%s ip=%s value=%d",
                            it->pattern.c_str(),
                            peername, it->value);
                session_timeout = it->value;
                break;
            }
    }
    timeout(session_timeout);
}

yazpp_1::IPDU_Observer *yf::FrontendNet::ZAssocChild::sessionNotify(
    yazpp_1::IPDU_Observable *the_PDU_Observable, int fd)
{
    return 0;
}

yf::FrontendNet::ZAssocChild::~ZAssocChild()
{
    int d = m_p->m_peerStat.remove(m_peer);
    if (m_p->m_msg_config.length())
    {
        std::ostringstream os;
        os  << m_p->m_msg_config << " "
            << m_peer << " closing cnt=" << d;
        yaz_log(YLOG_LOG, "%s", os.str().c_str());
    }
}

void yf::FrontendNet::ZAssocChild::report(Z_HTTP_Request *hreq)
{
    mp::odr o;

    Z_GDU *gdu_res = o.create_HTTP_Response(m_session, hreq, 200);

    Z_HTTP_Response *hres = gdu_res->u.HTTP_Response;

    mp::wrbuf w;
    size_t i;
    int number_total = 0;

    for (i = 0; m_p->m_duration_lim[i] != 0.0; i++)
        number_total += m_p->m_duration_freq[i];
    number_total += m_p->m_duration_freq[i];

    wrbuf_puts(w, "<?xml version=\"1.0\"?>\n");
    wrbuf_puts(w, "<frontend_net>\n");
    wrbuf_printf(w, "  <responses frequency=\"%d\">\n", number_total);
    for (i = 0; m_p->m_duration_lim[i] != 0.0; i++)
    {
        if (m_p->m_duration_freq[i] > 0)
            wrbuf_printf(
                w, "    <response duration_start=\"%f\" "
                "duration_end=\"%f\" frequency=\"%d\"/>\n",
                i > 0 ? m_p->m_duration_lim[i - 1] : 0.0,
                m_p->m_duration_lim[i], m_p->m_duration_freq[i]);
    }

    if (m_p->m_duration_freq[i] > 0)
        wrbuf_printf(
            w, "    <response duration_start=\"%f\" frequency=\"%d\"/>\n",
            m_p->m_duration_lim[i - 1], m_p->m_duration_freq[i]);

    if (m_p->m_duration_max != 0.0)
        wrbuf_printf(
            w, "    <response duration_max=\"%f\"/>\n",
            m_p->m_duration_max);
    if (m_p->m_duration_min != 0.0)
        wrbuf_printf(
            w, "    <response duration_min=\"%f\"/>\n",
            m_p->m_duration_min);
    if (m_p->m_duration_total != 0.0)
        wrbuf_printf(
            w, "    <response duration_average=\"%f\"/>\n",
            m_p->m_duration_total / number_total);

    wrbuf_puts(w, "  </responses>\n");

    int thread_busy;
    int thread_total;
    m_thread_pool_observer->get_thread_info(thread_busy, thread_total);

    wrbuf_printf(w, " <thread_info busy=\"%d\" total=\"%d\"/>\n",
                 thread_busy, thread_total);

    wrbuf_malloc_info(w);

    {
        char buf[200];
        if (nmem_get_status(buf, sizeof(buf) - 1) == 0)
            wrbuf_puts(w, buf);
    }
    wrbuf_puts(w, "</frontend_net>\n");

    hres->content_len = w.len();
    hres->content_buf = (char *) w.buf();

    int len;
    send_GDU(gdu_res, &len);
}

void yf::FrontendNet::ZAssocChild::recv_GDU(Z_GDU *z_pdu, int len)
{
    m_no_requests++;

    mp::Package *p = new mp::Package(m_session, m_origin);

    if (z_pdu && z_pdu->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request *hreq = z_pdu->u.HTTP_Request;

        const char *f = z_HTTP_header_lookup(hreq->headers, "X-Forwarded-For");
        if (f)
            p->origin().set_tcpip_address(std::string(f), m_session.id());

        if (m_p->m_stat_req.length()
            && !strcmp(hreq->path, m_p->m_stat_req.c_str()))
        {
            report(hreq);
            delete p;
            delete this;
            return;
        }
    }

    p->copy_route(*m_package);
    p->request() = yazpp_1::GDU(z_pdu);

    if (m_p->m_msg_config.length())
    {
        if (z_pdu)
        {
            std::ostringstream os;
            os  << m_p->m_msg_config << " "
                << *p << " "
                << "0.000000" << " "
                << *z_pdu;
            yaz_log(YLOG_LOG, "%s", os.str().c_str());
        }
    }
    if (z_pdu && z_pdu->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request *hreq = z_pdu->u.HTTP_Request;
        std::string peername = p->origin().get_address();

        m_limit_http_req.cleanup(false);
        int con_sz = m_limit_http_req.get_total(peername.c_str());
        std::list<IP_Pattern>::const_iterator it = m_p->http_req_max.begin();
        for (; it != m_p->http_req_max.end(); it++)
        {
            if (mp::util::match_ip(it->pattern, peername))
            {
                if (it->verbose > 1 ||
                    (it->value && con_sz >= it->value && it->verbose > 0))
                    yaz_log(YLOG_LOG, "http-req-max pattern=%s ip=%s con_sz=%d value=%d", it->pattern.c_str(), peername.c_str(), con_sz, it->value);
                if (con_sz < it->value)
                    break;
                mp::odr o;
                Z_GDU *gdu_res = o.create_HTTP_Response(m_session, hreq, 500);
                int len;
                send_GDU(gdu_res, &len);
                delete p;
                delete this;
                return;
            }
        }
        m_limit_http_req.add_connect(peername.c_str());
    }
    ThreadPoolPackage *tp = new ThreadPoolPackage(p, this, m_p);
    m_thread_pool_observer->put(tp);
}

void yf::FrontendNet::ZAssocChild::failNotify()
{
    // TODO: send Package to signal "close"
    if (m_session.is_closed())
    {
        if (m_no_requests == 0)
            delete this;
        return;
    }
    m_no_requests++;

    m_session.close();

    mp::Package *p = new mp::Package(m_session, m_origin);

    ThreadPoolPackage *tp = new ThreadPoolPackage(p, this, m_p);
    p->copy_route(*m_package);
    m_thread_pool_observer->cleanup(tp, &m_session);
    m_thread_pool_observer->put(tp);
}

void yf::FrontendNet::ZAssocChild::timeoutNotify()
{
    failNotify();
}

void yf::FrontendNet::ZAssocChild::connectNotify()
{

}

yf::FrontendNet::ZAssocServer::ZAssocServer(
    yazpp_1::IPDU_Observable *PDU_Observable,
    Port *port,
    Rep *rep)
    :
    Z_Assoc(PDU_Observable), m_port(port), m_p(rep)
{
    m_package = 0;
}


void yf::FrontendNet::ZAssocServer::set_package(const mp::Package *package)
{
    m_package = package;
}

void yf::FrontendNet::ZAssocServer::set_thread_pool(
    mp::ThreadPoolSocketObserver *observer)
{
    m_thread_pool_observer = observer;
}

yazpp_1::IPDU_Observer *yf::FrontendNet::ZAssocServer::sessionNotify(
    yazpp_1::IPDU_Observable *the_PDU_Observable, int fd)
{

    const char *peername = the_PDU_Observable->getpeername();
    if (!peername)
        peername = "unknown";
    else
    {
        const char *cp = strchr(peername, ':');
        if (cp)
            peername = cp + 1;
    }
    if (peername)
    {
        int total_sz = m_p->m_peerStat.get(peername);
        std::list<IP_Pattern>::const_iterator it = m_p->connect_total.begin();
        for (; it != m_p->connect_total.end(); it++)
        {
            if (mp::util::match_ip(it->pattern, peername))
            {
                if (it->verbose > 1 ||
                    (it->value && total_sz >= it->value && it->verbose > 0))
                    yaz_log(YLOG_LOG, "connect-total pattern=%s ip=%s con_sz=%d value=%d", it->pattern.c_str(), peername, total_sz, it->value);
                if (total_sz < it->value)
                    break;
                return 0;
            }
        }
        limit_connect.cleanup(false);
        int con_sz = limit_connect.get_total(peername);
        it = m_p->connect_max.begin();
        for (; it != m_p->connect_max.end(); it++)
        {
            if (mp::util::match_ip(it->pattern, peername))
            {
                if (it->verbose > 1 ||
                    (it->value && con_sz >= it->value && it->verbose > 0))
                    yaz_log(YLOG_LOG, "connect-max pattern=%s ip=%s con_sz=%d value=%d", it->pattern.c_str(), peername, con_sz, it->value);
                if (con_sz < it->value)
                    break;
                return 0;
            }
        }
        limit_connect.add_connect(peername);
    }
    ZAssocChild *my = new ZAssocChild(the_PDU_Observable,
                                      m_thread_pool_observer,
                                      m_package, m_port, m_p, limit_http_req,
                                      peername);
    return my;
}

yf::FrontendNet::ZAssocServer::~ZAssocServer()
{
}

void yf::FrontendNet::ZAssocServer::recv_GDU(Z_GDU *apdu, int len)
{
}

void yf::FrontendNet::ZAssocServer::failNotify()
{
}

void yf::FrontendNet::ZAssocServer::timeoutNotify()
{
}

void yf::FrontendNet::ZAssocServer::connectNotify()
{
}

yf::FrontendNet::FrontendNet() : m_p(new Rep)
{
}

yf::FrontendNet::Rep::Rep()
{
    m_max_threads = m_no_threads = 5;
    m_stack_size = 0;
    m_listen_duration = 0;
    az = 0;
    size_t i;
    for (i = 0; i < 22; i++)
        m_duration_freq[i] = 0;
    m_duration_lim[0] = 0.000001;
    m_duration_lim[1] = 0.00001;
    m_duration_lim[2] = 0.0001;
    m_duration_lim[3] = 0.001;
    m_duration_lim[4] = 0.01;
    m_duration_lim[5] = 0.1;
    m_duration_lim[6] = 0.2;
    m_duration_lim[7] = 0.3;
    m_duration_lim[8] = 0.5;
    m_duration_lim[9] = 1.0;
    m_duration_lim[10] = 1.5;
    m_duration_lim[11] = 2.0;
    m_duration_lim[12] = 3.0;
    m_duration_lim[13] = 4.0;
    m_duration_lim[14] = 5.0;
    m_duration_lim[15] = 6.0;
    m_duration_lim[16] = 8.0;
    m_duration_lim[17] = 10.0;
    m_duration_lim[18] = 15.0;
    m_duration_lim[19] = 20.0;
    m_duration_lim[20] = 30.0;
    m_duration_lim[21] = 0.0;
    m_duration_max = 0.0;
    m_duration_min = 0.0;
    m_duration_total = 0.0;
    m_stop_signo = 0;
}

yf::FrontendNet::Rep::~Rep()
{
    if (az)
    {
        size_t i;
        for (i = 0; i < m_ports.size(); i++)
            delete az[i];
        delete [] az;
        delete [] pdu;
    }
    az = 0;
}

yf::FrontendNet::~FrontendNet()
{
}

void yf::FrontendNet::stop(int signo) const
{
    m_p->m_stop_signo = signo;
}

void yf::FrontendNet::start() const
{
#if HAVE_GETRLIMIT
    struct rlimit limit_data;
    getrlimit(RLIMIT_NOFILE, &limit_data);
    yaz_log(YLOG_LOG, "getrlimit NOFILE cur=%ld max=%ld",
            (long) limit_data.rlim_cur, (long) limit_data.rlim_max);
#endif
}

bool yf::FrontendNet::My_Timer_Thread::timeout()
{
    return m_timeout;
}

yf::FrontendNet::My_Timer_Thread::My_Timer_Thread(
    yazpp_1::ISocketObservable *obs,
    int duration) :
    m_obs(obs), m_pipe(9123), m_timeout(false)
{
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
}

void yf::FrontendNet::My_Timer_Thread::socketNotify(int event)
{
    m_timeout = true;
    m_obs->deleteObserver(this);
}

void yf::FrontendNet::process(mp::Package &package) const
{
    if (m_p->az == 0)
        return;
    size_t i;
    My_Timer_Thread *tt = 0;

    if (m_p->m_listen_duration)
        tt = new My_Timer_Thread(&m_p->mySocketManager,
                                 m_p->m_listen_duration);

    ThreadPoolSocketObserver *tp =
        new ThreadPoolSocketObserver(&m_p->mySocketManager, m_p->m_no_threads,
                                     m_p->m_max_threads,
                                     m_p->m_stack_size);

    for (i = 0; i<m_p->m_ports.size(); i++)
    {
        yaz_log(YLOG_LOG, "listening on %s", m_p->m_ports[i].port.c_str());
        m_p->az[i]->set_package(&package);
        m_p->az[i]->set_thread_pool(tp);
    }
    while (m_p->mySocketManager.processEvent() > 0)
    {
        if (m_p->m_stop_signo == SIGTERM)
        {
            yaz_log(YLOG_LOG, "metaproxy received SIGTERM");
            if (m_p->az)
            {
                size_t i;
                for (i = 0; i < m_p->m_ports.size(); i++)
                {
                    m_p->pdu[i]->shutdown();
                    m_p->az[i]->server("");
                }
                yaz_daemon_stop();
            }
            return; // do not even attempt to destroy tp or tt
        }
#ifndef WIN32
        if (m_p->m_stop_signo == SIGUSR1)
        {    /* just stop listeners and cont till all sessions are done*/
            yaz_log(YLOG_LOG, "metaproxy received SIGUSR1");
            m_p->m_stop_signo = 0;
            if (m_p->az)
            {
                size_t i;
                for (i = 0; i < m_p->m_ports.size(); i++)
                    m_p->az[i]->server("");
                yaz_daemon_stop();
            }
        }
#endif
        int no = m_p->mySocketManager.getNumberOfObservers();
        if (no <= 1)
            break;
        if (tt && tt->timeout())
            break;
    }
    delete tp;
    delete tt;
}

void yf::FrontendNet::configure(const xmlNode * ptr, bool test_only,
                                const char *path)
{
    if (!ptr || !ptr->children)
    {
        throw yf::FilterException("No ports for Frontend");
    }
    std::vector<Port> ports;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "port"))
        {
            Port port;

            const char *names[5] = {"route", "max_recv_bytes", "port",
                                    "cert_fname", 0};
            std::string values[4];

            mp::xml::parse_attr(ptr, names, values);
            port.route = values[0];
            if (values[1].length() > 0)
                port.max_recv_bytes = atoi(values[1].c_str());
            else
                port.max_recv_bytes = 0;
            if (values[2].length() > 0)
                port.port = values[2];
            else
                port.port = mp::xml::get_text(ptr);
            port.cert_fname = values[3];
            ports.push_back(port);
        }
        else if (!strcmp((const char *) ptr->name, "threads"))
        {
            std::string threads_str = mp::xml::get_text(ptr);
            int threads = atoi(threads_str.c_str());
            if (threads < 1)
                throw yf::FilterException("Bad value for threads: "
                                                   + threads_str);
            m_p->m_no_threads = threads;
        }
        else if (!strcmp((const char *) ptr->name, "max-threads"))
        {
            std::string threads_str = mp::xml::get_text(ptr);
            int threads = atoi(threads_str.c_str());
            if (threads < 1)
                throw yf::FilterException("Bad value for max-threads: "
                                                   + threads_str);
            m_p->m_max_threads = threads;
        }
        else if (!strcmp((const char *) ptr->name, "stack-size"))
        {
            std::string sz_str = mp::xml::get_text(ptr);
            int sz = atoi(sz_str.c_str());
            if (sz < 0)
                throw yf::FilterException("Bad value for stack-size: "
                                                   + sz_str);
            m_p->m_stack_size = sz * 1024;
        }
        else if (!strcmp((const char *) ptr->name, "timeout"))
        {
            const char *names[3] = {"ip", "verbose", 0};
            std::string values[2];

            mp::xml::parse_attr(ptr, names, values);
            IP_Pattern m;
            m.value = mp::xml::get_int(ptr, 0);
            m.pattern = values[0];
            m.verbose = values[1].length() ? atoi(values[1].c_str()) : 1;
            m_p->session_timeout.push_back(m);
        }
        else if (!strcmp((const char *) ptr->name, "connect-max"))
        {
            const char *names[3] = {"ip", "verbose", 0};
            std::string values[2];

            mp::xml::parse_attr(ptr, names, values);
            IP_Pattern m;
            m.value = mp::xml::get_int(ptr, INT_MAX);
            m.pattern = values[0];
            m.verbose = values[1].length() ? atoi(values[1].c_str()) : 1;
            m_p->connect_max.push_back(m);
        }
        else if (!strcmp((const char *) ptr->name, "connect-total"))
        {
            const char *names[3] = {"ip", "verbose", 0};
            std::string values[2];

            mp::xml::parse_attr(ptr, names, values);
            IP_Pattern m;
            m.value = mp::xml::get_int(ptr, INT_MAX);
            m.pattern = values[0];
            m.verbose = values[1].length() ? atoi(values[1].c_str()) : 1;
            m_p->connect_total.push_back(m);
        }
        else if (!strcmp((const char *) ptr->name, "http-req-max"))
        {
            const char *names[3] = {"ip", "verbose", 0};
            std::string values[2];

            mp::xml::parse_attr(ptr, names, values);
            IP_Pattern m;
            m.value = mp::xml::get_int(ptr, INT_MAX);
            m.pattern = values[0];
            m.verbose = values[1].length() ? atoi(values[1].c_str()) : 1;
            m_p->http_req_max.push_back(m);
        }
        else if (!strcmp((const char *) ptr->name, "message"))
        {
            m_p->m_msg_config = mp::xml::get_text(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "stat-req"))
        {
            m_p->m_stat_req = mp::xml::get_text(ptr);
        }
        else
        {
            throw yf::FilterException("Bad element "
                                      + std::string((const char *)
                                                    ptr->name));
        }
    }
    if (m_p->m_msg_config.length() > 0 && m_p->m_stat_req.length() == 0)
    {   // allow stats if message is enabled for filter
        m_p->m_stat_req = "/fn_stat";
    }
    if (test_only)
        return;
    set_ports(ports);
}

void yf::FrontendNet::set_ports(std::vector<std::string> &ports)
{
    std::vector<Port> nports;
    size_t i;

    for (i = 0; i < ports.size(); i++)
    {
        Port nport;

        nport.port = ports[i];

        nports.push_back(nport);
    }
    set_ports(nports);
}


void yf::FrontendNet::set_ports(std::vector<Port> &ports)
{
    m_p->m_ports = ports;

    m_p->az = new yf::FrontendNet::ZAssocServer *[m_p->m_ports.size()];
    m_p->pdu = new yazpp_1::PDU_Assoc *[m_p->m_ports.size()];

    // Create yf::FrontendNet::ZAssocServer for each port
    size_t i;
    for (i = 0; i < m_p->m_ports.size(); i++)
        m_p->az[i] = 0;
    for (i = 0; i < m_p->m_ports.size(); i++)
    {
        // create a PDU assoc object (one per yf::FrontendNet::ZAssocServer)
        yazpp_1::PDU_Assoc *as = new yazpp_1::PDU_Assoc(&m_p->mySocketManager);

        if (m_p->m_ports[i].cert_fname.length())
            as->set_cert_fname(m_p->m_ports[i].cert_fname.c_str());
        // create ZAssoc with PDU Assoc
        m_p->pdu[i] = as;
        m_p->az[i] = new yf::FrontendNet::ZAssocServer(
            as, &m_p->m_ports[i], m_p.get());
        if (m_p->az[i]->server(m_p->m_ports[i].port.c_str()))
        {
            throw yf::FilterException("Unable to bind to address "
                                      + std::string(m_p->m_ports[i].port));
        }
        COMSTACK cs = as->get_comstack();

        if (cs && m_p->m_ports[i].max_recv_bytes)
            cs_set_max_recv_bytes(cs, m_p->m_ports[i].max_recv_bytes);

    }
}

void yf::FrontendNet::set_listen_duration(int d)
{
    m_p->m_listen_duration = d;
}


yf::FrontendNet::PeerStat::PeerStat()
{
    items = 0;
}

yf::FrontendNet::PeerStat::~PeerStat()
{
    while (items)
    {
        Item *n = items->next;
        delete items;
        items = n;
    }
}


size_t yf::FrontendNet::PeerStat::add(const std::string &peer)
{
    Item *n = items;
    for (; n; n = n->next)
    {
        if (peer == n->m_peer)
        {
            n->cnt++;
            return n->cnt;
        }
    }
    n = new Item();
    n->cnt = 1;
    n->m_peer = peer;
    n->next = items;
    items = n;
    return n->cnt;
}

size_t yf::FrontendNet::PeerStat::get(const std::string &peer)
{
    Item *n = items;
    for (; n; n = n->next)
    {
        if (peer == n->m_peer)
            return n->cnt;
    }
    return 0;
}

size_t yf::FrontendNet::PeerStat::remove(const std::string &peer)
{
    Item **np = &items;
    for (; *np; np = &(*np)->next)
    {
        Item *n = *np;
        if (peer == n->m_peer)
        {
            if (--n->cnt == 0)
            {
                *np = n->next;
                delete n;
                return 0;
            }
            return n->cnt;
        }
    }
    return 0;
}

static yf::Base* filter_creator()
{
    return new yf::FrontendNet;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_frontend_net = {
        0,
        "frontend_net",
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

