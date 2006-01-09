/* $Id: filter_frontend_net.cpp,v 1.14 2006-01-09 21:19:11 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "xmlutil.hpp"
#include "pipe.hpp"
#include "filter.hpp"
#include "package.hpp"
#include "thread_pool_observer.hpp"
#include "filter_frontend_net.hpp"
#include <yaz++/z-assoc.h>
#include <yaz++/pdu-assoc.h>
#include <yaz++/socket-manager.h>
#include <yaz/log.h>

#include <iostream>

namespace yp2 {
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
                          yp2::ThreadPoolSocketObserver *m_thread_pool_observer,
                          const yp2::Package *package);
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
        yp2::ThreadPoolSocketObserver *m_thread_pool_observer;
        yp2::Session m_session;
        yp2::Origin m_origin;
        bool m_delete_flag;
        const yp2::Package *m_package;
    };
    class ThreadPoolPackage : public yp2::IThreadPoolMsg {
    public:
        ThreadPoolPackage(yp2::Package *package, yp2::ZAssocChild *ses) :
            m_session(ses), m_package(package) { };
        ~ThreadPoolPackage();
        IThreadPoolMsg *handle();
        void result();
        
    private:
        yp2::ZAssocChild *m_session;
        yp2::Package *m_package;
        
    };
    class ZAssocServer : public yazpp_1::Z_Assoc {
    public:
        ~ZAssocServer();
        ZAssocServer(yazpp_1::IPDU_Observable *PDU_Observable,
                     yp2::ThreadPoolSocketObserver *m_thread_pool_observer,
                     const yp2::Package *package);
    private:
        yazpp_1::IPDU_Observer* sessionNotify(
            yazpp_1::IPDU_Observable *the_PDU_Observable,
            int fd);
        void recv_GDU(Z_GDU *apdu, int len);
        
        void failNotify();
        void timeoutNotify();
    void connectNotify();
    private:
        yp2::ThreadPoolSocketObserver *m_thread_pool_observer;
        const yp2::Package *m_package;
    };
}

yp2::ThreadPoolPackage::~ThreadPoolPackage()
{
    delete m_package;
}

void yp2::ThreadPoolPackage::result()
{
    m_session->m_no_requests--;

    yazpp_1::GDU *gdu = &m_package->response();
    if (gdu->get())
    {
	int len;
	m_session->send_GDU(gdu->get(), &len);
    }
    if (m_session->m_no_requests == 0 && m_package->session().is_closed())
	delete m_session;
    delete this;
}

yp2::IThreadPoolMsg *yp2::ThreadPoolPackage::handle() 
{
    m_package->move();
    return this;
}


yp2::ZAssocChild::ZAssocChild(yazpp_1::IPDU_Observable *PDU_Observable,
				     yp2::ThreadPoolSocketObserver *my_thread_pool,
				     const yp2::Package *package)
    :  Z_Assoc(PDU_Observable)
{
    m_thread_pool_observer = my_thread_pool;
    m_no_requests = 0;
    m_delete_flag = false;
    m_package = package;
}


yazpp_1::IPDU_Observer *yp2::ZAssocChild::sessionNotify(yazpp_1::IPDU_Observable
						  *the_PDU_Observable, int fd)
{
    return 0;
}

yp2::ZAssocChild::~ZAssocChild()
{
}

void yp2::ZAssocChild::recv_GDU(Z_GDU *z_pdu, int len)
{
    m_no_requests++;

    yp2::Package *p = new yp2::Package(m_session, m_origin);

    yp2::ThreadPoolPackage *tp = new yp2::ThreadPoolPackage(p, this);
    p->copy_filter(*m_package);
    p->request() = yazpp_1::GDU(z_pdu);
    m_thread_pool_observer->put(tp);  
}

void yp2::ZAssocChild::failNotify()
{
    // TODO: send Package to signal "close"
    if (m_session.is_closed())
	return;
    m_no_requests++;

    m_session.close();

    yp2::Package *p = new yp2::Package(m_session, m_origin);

    yp2::ThreadPoolPackage *tp = new yp2::ThreadPoolPackage(p, this);
    p->copy_filter(*m_package);
    m_thread_pool_observer->put(tp);  
}

void yp2::ZAssocChild::timeoutNotify()
{
    failNotify();
}

void yp2::ZAssocChild::connectNotify()
{

}

yp2::ZAssocServer::ZAssocServer(yazpp_1::IPDU_Observable *PDU_Observable,
			   yp2::ThreadPoolSocketObserver *thread_pool_observer,
			   const yp2::Package *package)
    :  Z_Assoc(PDU_Observable)
{
    m_thread_pool_observer = thread_pool_observer;
    m_package = package;

}

yazpp_1::IPDU_Observer *yp2::ZAssocServer::sessionNotify(yazpp_1::IPDU_Observable
						 *the_PDU_Observable, int fd)
{
    yp2::ZAssocChild *my =
	new yp2::ZAssocChild(the_PDU_Observable, m_thread_pool_observer,
                             m_package);
    return my;
}

yp2::ZAssocServer::~ZAssocServer()
{
}

void yp2::ZAssocServer::recv_GDU(Z_GDU *apdu, int len)
{
}

void yp2::ZAssocServer::failNotify()
{
}

void yp2::ZAssocServer::timeoutNotify()
{
}

void yp2::ZAssocServer::connectNotify()
{
}

yp2::filter::FrontendNet::FrontendNet() : m_p(new Rep)
{
    m_p->m_no_threads = 5;
    m_p->m_listen_duration = 0;
}

yp2::filter::FrontendNet::~FrontendNet()
{
}

bool yp2::My_Timer_Thread::timeout()
{
    return m_timeout;
}

yp2::My_Timer_Thread::My_Timer_Thread(yazpp_1::ISocketObservable *obs,
				 int duration) : 
    m_obs(obs), m_pipe(9123), m_timeout(false)
{
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
}

void yp2::My_Timer_Thread::socketNotify(int event)
{
    m_timeout = true;
    m_obs->deleteObserver(this);
}

void yp2::filter::FrontendNet::process(Package &package) const
{
    if (m_p->m_ports.size() == 0)
        return;

    yazpp_1::SocketManager mySocketManager;

    My_Timer_Thread *tt = 0;
    if (m_p->m_listen_duration)
	tt = new My_Timer_Thread(&mySocketManager, m_p->m_listen_duration);

    ThreadPoolSocketObserver threadPool(&mySocketManager, m_p->m_no_threads);

    yp2::ZAssocServer **az = new yp2::ZAssocServer *[m_p->m_ports.size()];

    // Create yp2::ZAssocServer for each port
    size_t i;
    for (i = 0; i<m_p->m_ports.size(); i++)
    {
	// create a PDU assoc object (one per yp2::ZAssocServer)
	yazpp_1::PDU_Assoc *as = new yazpp_1::PDU_Assoc(&mySocketManager);

	// create ZAssoc with PDU Assoc
	az[i] = new yp2::ZAssocServer(as, &threadPool, &package);
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

void yp2::filter::FrontendNet::configure(const xmlNode * ptr)
{
    if (!ptr || !ptr->children)
    {
        throw yp2::filter::FilterException("No ports for Frontend");
    }
    std::vector<std::string> ports;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "port"))
        {
            std::string port = yp2::xml::get_text(ptr);
            ports.push_back(port);
            
        }
        else if (!strcmp((const char *) ptr->name, "threads"))
        {
            std::string threads_str = yp2::xml::get_text(ptr);
            int threads = atoi(threads_str.c_str());
            if (threads < 1)
                throw yp2::filter::FilterException("Bad value for threads: " 
                                                   + threads_str);
            m_p->m_no_threads = threads;
        }
        else
        {
            throw yp2::filter::FilterException("Bad element " 
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
    m_p->m_ports = ports;
}

std::vector<std::string> &yp2::filter::FrontendNet::ports()
{
    return m_p->m_ports;
}

int &yp2::filter::FrontendNet::listen_duration()
{
    return m_p->m_listen_duration;
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::FrontendNet;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_frontend_net = {
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
