/* $Id: filter_frontend_net.cpp,v 1.20 2006-08-30 09:56:41 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include "util.hpp"
#include "pipe.hpp"
#include "filter.hpp"
#include "package.hpp"
#include "thread_pool_observer.hpp"
#include "filter_frontend_net.hpp"
#include <yazpp/z-assoc.h>
#include <yazpp/pdu-assoc.h>
#include <yazpp/socket-manager.h>
#include <yaz/log.h>

#include <iostream>

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class FrontendNet::Rep {
            friend class FrontendNet;
            int m_no_threads;
            std::vector<std::string> m_ports;
            int m_listen_duration;
        };
    }
    class My_Timer_Thread : public yazpp_1::ISocketObserver {
    private:
        yazpp_1::ISocketObservable *m_obs;
        Pipe m_pipe;
        bool m_timeout;
    public:
        My_Timer_Thread(yazpp_1::ISocketObservable *obs, int duration);
        void socketNotify(int event);
        bool timeout();
    };
    class ZAssocChild : public yazpp_1::Z_Assoc {
    public:
        ~ZAssocChild();
        ZAssocChild(yazpp_1::IPDU_Observable *the_PDU_Observable,
                          mp::ThreadPoolSocketObserver *m_thread_pool_observer,
                          const mp::Package *package);
        int m_no_requests;
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
        mp::Session m_session;
        mp::Origin m_origin;
        bool m_delete_flag;
        const mp::Package *m_package;
    };
    class ThreadPoolPackage : public mp::IThreadPoolMsg {
    public:
        ThreadPoolPackage(mp::Package *package, mp::ZAssocChild *ses) :
            m_session(ses), m_package(package) { };
        ~ThreadPoolPackage();
        IThreadPoolMsg *handle();
        void result();
        
    private:
        mp::ZAssocChild *m_session;
        mp::Package *m_package;
        
    };
    class ZAssocServer : public yazpp_1::Z_Assoc {
    public:
        ~ZAssocServer();
        ZAssocServer(yazpp_1::IPDU_Observable *PDU_Observable,
                     mp::ThreadPoolSocketObserver *m_thread_pool_observer,
                     const mp::Package *package);
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
    };
}

mp::ThreadPoolPackage::~ThreadPoolPackage()
{
    delete m_package;
}

void mp::ThreadPoolPackage::result()
{
    m_session->m_no_requests--;

    yazpp_1::GDU *gdu = &m_package->response();

    if (gdu->get())
    {
	int len;
	m_session->send_GDU(gdu->get(), &len);
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

            m_session->send_Z_PDU(apdu_response, &len);
            m_package->session().close();
        }
    }

    if (m_session->m_no_requests == 0 && m_package->session().is_closed())
	delete m_session;
    delete this;
}

mp::IThreadPoolMsg *mp::ThreadPoolPackage::handle() 
{
    m_package->move();
    return this;
}


mp::ZAssocChild::ZAssocChild(yazpp_1::IPDU_Observable *PDU_Observable,
				     mp::ThreadPoolSocketObserver *my_thread_pool,
				     const mp::Package *package)
    :  Z_Assoc(PDU_Observable)
{
    m_thread_pool_observer = my_thread_pool;
    m_no_requests = 0;
    m_delete_flag = false;
    m_package = package;
    // TODO why is m_origin not set here someplace ?? MC ??
}


yazpp_1::IPDU_Observer *mp::ZAssocChild::sessionNotify(yazpp_1::IPDU_Observable
						  *the_PDU_Observable, int fd)
{
    return 0;
}

mp::ZAssocChild::~ZAssocChild()
{
}

void mp::ZAssocChild::recv_GDU(Z_GDU *z_pdu, int len)
{
    m_no_requests++;

    mp::Package *p = new mp::Package(m_session, m_origin);

    mp::ThreadPoolPackage *tp = new mp::ThreadPoolPackage(p, this);
    p->copy_filter(*m_package);
    p->request() = yazpp_1::GDU(z_pdu);
    m_thread_pool_observer->put(tp);  
}

void mp::ZAssocChild::failNotify()
{
    // TODO: send Package to signal "close"
    if (m_session.is_closed())
	return;
    m_no_requests++;

    m_session.close();

    mp::Package *p = new mp::Package(m_session, m_origin);

    mp::ThreadPoolPackage *tp = new mp::ThreadPoolPackage(p, this);
    p->copy_filter(*m_package);
    m_thread_pool_observer->put(tp);  
}

void mp::ZAssocChild::timeoutNotify()
{
    failNotify();
}

void mp::ZAssocChild::connectNotify()
{

}

mp::ZAssocServer::ZAssocServer(yazpp_1::IPDU_Observable *PDU_Observable,
			   mp::ThreadPoolSocketObserver *thread_pool_observer,
			   const mp::Package *package)
    :  Z_Assoc(PDU_Observable)
{
    m_thread_pool_observer = thread_pool_observer;
    m_package = package;

}

yazpp_1::IPDU_Observer *mp::ZAssocServer::sessionNotify(yazpp_1::IPDU_Observable
						 *the_PDU_Observable, int fd)
{
    mp::ZAssocChild *my =
	new mp::ZAssocChild(the_PDU_Observable, m_thread_pool_observer,
                             m_package);
    return my;
}

mp::ZAssocServer::~ZAssocServer()
{
}

void mp::ZAssocServer::recv_GDU(Z_GDU *apdu, int len)
{
}

void mp::ZAssocServer::failNotify()
{
}

void mp::ZAssocServer::timeoutNotify()
{
}

void mp::ZAssocServer::connectNotify()
{
}

mp::filter::FrontendNet::FrontendNet() : m_p(new Rep)
{
    m_p->m_no_threads = 5;
    m_p->m_listen_duration = 0;
}

mp::filter::FrontendNet::~FrontendNet()
{
}

bool mp::My_Timer_Thread::timeout()
{
    return m_timeout;
}

mp::My_Timer_Thread::My_Timer_Thread(yazpp_1::ISocketObservable *obs,
				 int duration) : 
    m_obs(obs), m_pipe(9123), m_timeout(false)
{
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
}

void mp::My_Timer_Thread::socketNotify(int event)
{
    m_timeout = true;
    m_obs->deleteObserver(this);
}

void mp::filter::FrontendNet::process(Package &package) const
{
    if (m_p->m_ports.size() == 0)
        return;

    yazpp_1::SocketManager mySocketManager;

    My_Timer_Thread *tt = 0;
    if (m_p->m_listen_duration)
	tt = new My_Timer_Thread(&mySocketManager, m_p->m_listen_duration);

    ThreadPoolSocketObserver threadPool(&mySocketManager, m_p->m_no_threads);

    mp::ZAssocServer **az = new mp::ZAssocServer *[m_p->m_ports.size()];

    // Create mp::ZAssocServer for each port
    size_t i;
    for (i = 0; i<m_p->m_ports.size(); i++)
    {
	// create a PDU assoc object (one per mp::ZAssocServer)
	yazpp_1::PDU_Assoc *as = new yazpp_1::PDU_Assoc(&mySocketManager);

	// create ZAssoc with PDU Assoc
	az[i] = new mp::ZAssocServer(as, &threadPool, &package);
	az[i]->server(m_p->m_ports[i].c_str());
    }
    while (mySocketManager.processEvent() > 0)
    {
	if (tt && tt->timeout())
	    break;
    }
    for (i = 0; i<m_p->m_ports.size(); i++)
	delete az[i];

    delete [] az;
    delete tt;
}

void mp::filter::FrontendNet::configure(const xmlNode * ptr)
{
    if (!ptr || !ptr->children)
    {
        throw mp::filter::FilterException("No ports for Frontend");
    }
    std::vector<std::string> ports;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "port"))
        {
            std::string port = mp::xml::get_text(ptr);
            ports.push_back(port);
            
        }
        else if (!strcmp((const char *) ptr->name, "threads"))
        {
            std::string threads_str = mp::xml::get_text(ptr);
            int threads = atoi(threads_str.c_str());
            if (threads < 1)
                throw mp::filter::FilterException("Bad value for threads: " 
                                                   + threads_str);
            m_p->m_no_threads = threads;
        }
        else
        {
            throw mp::filter::FilterException("Bad element " 
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
    m_p->m_ports = ports;
}

std::vector<std::string> &mp::filter::FrontendNet::ports()
{
    return m_p->m_ports;
}

int &mp::filter::FrontendNet::listen_duration()
{
    return m_p->m_listen_duration;
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::FrontendNet;
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
