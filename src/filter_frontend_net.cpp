/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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
#include "gduutil.hpp"

#include <iostream>

namespace mp = metaproxy_1;
namespace yf = metaproxy_1::filter;

namespace metaproxy_1 {
    namespace filter {
        class FrontendNet::Port {
            friend class Rep;
            friend class FrontendNet;
            std::string port;
            std::string route;
        };
        class FrontendNet::Rep {
            friend class FrontendNet;

            int m_no_threads;
            std::vector<Port> m_ports;
            int m_listen_duration;
            int m_session_timeout;
            int m_connect_max;
            std::string m_msg_config;
            std::string m_stat_req;
            yazpp_1::SocketManager mySocketManager;
            ZAssocServer **az;
            int m_duration_freq[22];
            double m_duration_lim[22];
            double m_duration_max;
            double m_duration_min;
            double m_duration_total;
            bool m_stop;
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
                        std::string route,
                        Rep *rep);
            int m_no_requests;
            std::string m_route;
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
                         std::string route,
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
            std::string m_route;
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

    return *ses == m_package->session();
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
    m_package->move(m_assoc_child->m_route);
    return this;
}

yf::FrontendNet::ZAssocChild::ZAssocChild(
    yazpp_1::IPDU_Observable *PDU_Observable,
    mp::ThreadPoolSocketObserver *my_thread_pool,
    const mp::Package *package,
    std::string route, Rep *rep)
    :  Z_Assoc(PDU_Observable), m_p(rep)
{
    m_thread_pool_observer = my_thread_pool;
    m_no_requests = 0;
    m_delete_flag = false;
    m_package = package;
    m_route = route;
    const char *peername = PDU_Observable->getpeername();
    if (!peername)
        peername = "unknown";
    m_origin.set_tcpip_address(std::string(peername), m_session.id());
    timeout(m_p->m_session_timeout);
}

yazpp_1::IPDU_Observer *yf::FrontendNet::ZAssocChild::sessionNotify(
    yazpp_1::IPDU_Observable *the_PDU_Observable, int fd)
{
    return 0;
}

yf::FrontendNet::ZAssocChild::~ZAssocChild()
{
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

    wrbuf_puts(w, " </responses>\n");

    int thread_busy;
    int thread_total;
    m_thread_pool_observer->get_thread_info(thread_busy, thread_total);

    wrbuf_printf(w, " <thread_info busy=\"%d\" total=\"%d\"/>\n",
                 thread_busy, thread_total);

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
            return;
        }
    }

    ThreadPoolPackage *tp = new ThreadPoolPackage(p, this, m_p);
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
    std::string route,
    Rep *rep)
    :
    Z_Assoc(PDU_Observable), m_route(route), m_p(rep)
{
    m_package = 0;
}


void yf::FrontendNet::ZAssocServer::set_package(const mp::Package *package)
{
    m_package = package;
}

void yf::FrontendNet::ZAssocServer::set_thread_pool(
    ThreadPoolSocketObserver *observer)
{
    m_thread_pool_observer = observer;
}

yazpp_1::IPDU_Observer *yf::FrontendNet::ZAssocServer::sessionNotify(
    yazpp_1::IPDU_Observable *the_PDU_Observable, int fd)
{

    const char *peername = the_PDU_Observable->getpeername();
    if (peername)
    {
        limit_connect.add_connect(peername);
        limit_connect.cleanup(false);
        int con_sz = limit_connect.get_total(peername);
        if (m_p->m_connect_max && con_sz > m_p->m_connect_max)
            return 0;
    }
    ZAssocChild *my = new ZAssocChild(the_PDU_Observable,
                                      m_thread_pool_observer,
                                      m_package, m_route, m_p);
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
    m_no_threads = 5;
    m_listen_duration = 0;
    m_session_timeout = 300; // 5 minutes
    m_connect_max = 0;
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
    m_stop = false;
}

yf::FrontendNet::Rep::~Rep()
{
    if (az)
    {
        size_t i;
        for (i = 0; i < m_ports.size(); i++)
            delete az[i];
        delete [] az;
    }
    az = 0;
}

yf::FrontendNet::~FrontendNet()
{
}

void yf::FrontendNet::stop() const
{
    m_p->m_stop = true;
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

void yf::FrontendNet::process(Package &package) const
{
    if (m_p->az == 0)
        return;
    size_t i;
    My_Timer_Thread *tt = 0;

    if (m_p->m_listen_duration)
        tt = new My_Timer_Thread(&m_p->mySocketManager,
                                 m_p->m_listen_duration);

    ThreadPoolSocketObserver tp(&m_p->mySocketManager, m_p->m_no_threads);

    for (i = 0; i<m_p->m_ports.size(); i++)
    {
	m_p->az[i]->set_package(&package);
	m_p->az[i]->set_thread_pool(&tp);
    }
    while (m_p->mySocketManager.processEvent() > 0)
    {
        if (m_p->m_stop)
        {
            m_p->m_stop = false;
            if (m_p->az)
            {
                size_t i;
                for (i = 0; i < m_p->m_ports.size(); i++)
                    m_p->az[i]->server("");
                yaz_daemon_stop();
            }
        }
        int no = m_p->mySocketManager.getNumberOfObservers();
        if (no <= 1)
            break;
	if (tt && tt->timeout())
	    break;
    }
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
            port.route = mp::xml::get_route(ptr);
            port.port = mp::xml::get_text(ptr);
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
        else if (!strcmp((const char *) ptr->name, "timeout"))
        {
            std::string timeout_str = mp::xml::get_text(ptr);
            int timeout = atoi(timeout_str.c_str());
            if (timeout < 1)
                throw yf::FilterException("Bad value for timeout: "
                                                   + timeout_str);
            m_p->m_session_timeout = timeout;
        }
        else if (!strcmp((const char *) ptr->name, "connect-max"))
        {
            m_p->m_connect_max = mp::xml::get_int(ptr, 0);
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

    // Create yf::FrontendNet::ZAssocServer for each port
    size_t i;
    for (i = 0; i<m_p->m_ports.size(); i++)
    {
        // create a PDU assoc object (one per yf::FrontendNet::ZAssocServer)
        yazpp_1::PDU_Assoc *as = new yazpp_1::PDU_Assoc(&m_p->mySocketManager);

        // create ZAssoc with PDU Assoc
        m_p->az[i] = new yf::FrontendNet::ZAssocServer(
            as, m_p->m_ports[i].route, m_p.get());
        if (m_p->az[i]->server(m_p->m_ports[i].port.c_str()))
        {
            throw yf::FilterException("Unable to bind to address "
                                      + std::string(m_p->m_ports[i].port));
        }
    }
}

void yf::FrontendNet::set_listen_duration(int d)
{
    m_p->m_listen_duration = d;
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

