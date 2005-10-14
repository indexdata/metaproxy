

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"
#include "thread_pool_observer.hpp"
#include "filter_frontend_net.hpp"
#include <yaz++/z-assoc.h>
#include <yaz++/pdu-assoc.h>
#include <yaz++/socket-manager.h>
#include <yaz/log.h>

#include <iostream>

using namespace yp2;

class ZAssocServerChild : public yazpp_1::Z_Assoc {
public:
    ~ZAssocServerChild();
    ZAssocServerChild(yazpp_1::IPDU_Observable *the_PDU_Observable,
	       ThreadPoolSocketObserver *m_thread_pool_observer,
	       const Package *package);
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
    ThreadPoolSocketObserver *m_thread_pool_observer;
    Session m_session;
    Origin m_origin;
    bool m_delete_flag;
    const Package *m_package;
};


class ThreadPoolPackage : public IThreadPoolMsg {
public:
    ThreadPoolPackage(Package *package, ZAssocServerChild *ses) :
	m_session(ses), m_package(package) { };
    ~ThreadPoolPackage();
    IThreadPoolMsg *handle();
    void result();
    
private:
    ZAssocServerChild *m_session;
    Package *m_package;
    
};

ThreadPoolPackage::~ThreadPoolPackage()
{
    delete m_package;
}

void ThreadPoolPackage::result()
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

IThreadPoolMsg *ThreadPoolPackage::handle() 
{
    m_package->move();
    return this;
}


ZAssocServerChild::ZAssocServerChild(yazpp_1::IPDU_Observable *the_PDU_Observable,
		       ThreadPoolSocketObserver *my_thread_pool,
		       const Package *package)
    :  Z_Assoc(the_PDU_Observable)
{
    m_thread_pool_observer = my_thread_pool;
    m_no_requests = 0;
    m_delete_flag = false;
    m_package = package;
}


yazpp_1::IPDU_Observer *ZAssocServerChild::sessionNotify(yazpp_1::IPDU_Observable
						  *the_PDU_Observable, int fd)
{
    return 0;
}

ZAssocServerChild::~ZAssocServerChild()
{
}

void ZAssocServerChild::recv_GDU(Z_GDU *z_pdu, int len)
{
    m_no_requests++;

    Package *p = new Package(m_session, m_origin);

    ThreadPoolPackage *tp = new ThreadPoolPackage(p, this);
    p->copy_filter(*m_package);
    p->request() = yazpp_1::GDU(z_pdu);
    m_thread_pool_observer->put(tp);  
}

void ZAssocServerChild::failNotify()
{
    // TODO: send Package to signal "close"
    if (m_session.is_closed())
	return;
    m_no_requests++;

    m_session.close();

    Package *p = new Package(m_session, m_origin);

    ThreadPoolPackage *tp = new ThreadPoolPackage(p, this);
    p->copy_filter(*m_package);
    m_thread_pool_observer->put(tp);  
}

void ZAssocServerChild::timeoutNotify()
{
    failNotify();
}

void ZAssocServerChild::connectNotify()
{

}

class ZAssocServer : public yazpp_1::Z_Assoc {
public:
    ~ZAssocServer();
    ZAssocServer(yazpp_1::IPDU_Observable *the_PDU_Observable,
              ThreadPoolSocketObserver *m_thread_pool_observer,
	      const Package *package);
private:
    yazpp_1::IPDU_Observer* sessionNotify(
        yazpp_1::IPDU_Observable *the_PDU_Observable,
        int fd);
    void recv_GDU(Z_GDU *apdu, int len);
    
    void failNotify();
    void timeoutNotify();
    void connectNotify();
private:
    ThreadPoolSocketObserver *m_thread_pool_observer;
    const Package *m_package;
};


ZAssocServer::ZAssocServer(yazpp_1::IPDU_Observable *the_PDU_Observable,
                     ThreadPoolSocketObserver *thread_pool_observer,
		     const Package *package)
    :  Z_Assoc(the_PDU_Observable)
{
    m_thread_pool_observer = thread_pool_observer;
    m_package = package;

}

yazpp_1::IPDU_Observer *ZAssocServer::sessionNotify(yazpp_1::IPDU_Observable
						 *the_PDU_Observable, int fd)
{
    ZAssocServerChild *my =
	new ZAssocServerChild(the_PDU_Observable, m_thread_pool_observer,
			      m_package);
    return my;
}

ZAssocServer::~ZAssocServer()
{
}

void ZAssocServer::recv_GDU(Z_GDU *apdu, int len)
{
}

void ZAssocServer::failNotify()
{
}

void ZAssocServer::timeoutNotify()
{
}

void ZAssocServer::connectNotify()
{
}

FilterFrontendNet::FilterFrontendNet()
{
    m_no_threads = 5;
    m_listen_duration = 0;
}

class My_Timer_Thread : public yazpp_1::ISocketObserver {
private:
    yazpp_1::ISocketObservable *m_obs;
    int m_fd[2];
    bool m_timeout;
public:
    My_Timer_Thread(yazpp_1::ISocketObservable *obs, int duration);
    void socketNotify(int event);
    bool timeout();
};

bool My_Timer_Thread::timeout()
{
    return m_timeout;
}

My_Timer_Thread::My_Timer_Thread(yazpp_1::ISocketObservable *obs,
				 int duration) : 
    m_obs(obs), m_timeout(false)
{
    pipe(m_fd);
    obs->addObserver(m_fd[0], this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
}

void My_Timer_Thread::socketNotify(int event)
{
    m_timeout = true;
    m_obs->deleteObserver(this);
    close(m_fd[0]);
    close(m_fd[1]);
}

void FilterFrontendNet::process(Package &package) const {
    yazpp_1::SocketManager mySocketManager;

    My_Timer_Thread *tt = 0;
    if (m_listen_duration)
	tt = new My_Timer_Thread(&mySocketManager, m_listen_duration);

    ThreadPoolSocketObserver threadPool(&mySocketManager, m_no_threads);

    ZAssocServer **az = new ZAssocServer *[m_ports.size()];

    // Create ZAssocServer for each port
    size_t i;
    for (i = 0; i<m_ports.size(); i++)
    {
	// create a PDU assoc object (one per ZAssocServer)
	yazpp_1::PDU_Assoc *as = new yazpp_1::PDU_Assoc(&mySocketManager);

	// create ZAssoc with PDU Assoc
	az[i] = new ZAssocServer(as, &threadPool, &package);
	az[i]->server(m_ports[i].c_str());
    }
    while (mySocketManager.processEvent() > 0)
    {
	if (tt && tt->timeout())
	    break;
    }
    for (i = 0; i<m_ports.size(); i++)
	delete az[i];

    delete [] az;
    delete tt;
}

std::vector<std::string> &FilterFrontendNet::ports()
{
    return m_ports;
}

int &FilterFrontendNet::listen_duration()
{
    return m_listen_duration;
}

