/* $Id: filter_z3950_client.cpp,v 1.12 2006-01-02 14:33:42 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"
#include "util.hpp"
#include "filter_z3950_client.hpp"

#include <map>
#include <stdexcept>
#include <list>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <yaz/zgdu.h>
#include <yaz/log.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <yaz++/socket-manager.h>
#include <yaz++/pdu-assoc.h>
#include <yaz++/z-assoc.h>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class Z3950Client::Assoc : public yazpp_1::Z_Assoc{
            friend class Rep;
            Assoc(yazpp_1::SocketManager *socket_manager,
                  yazpp_1::IPDU_Observable *PDU_Observable,
                  std::string host);
            ~Assoc();
            void connectNotify();
            void failNotify();
            void timeoutNotify();
            void recv_GDU(Z_GDU *gdu, int len);
            yazpp_1::IPDU_Observer* sessionNotify(
                yazpp_1::IPDU_Observable *the_PDU_Observable,
                int fd);

            yazpp_1::SocketManager *m_socket_manager;
            yazpp_1::IPDU_Observable *m_PDU_Observable;
            Package *m_package;
            bool m_in_use;
            bool m_waiting;
            bool m_destroyed;
            bool m_connected;
            std::string m_host;
        };

        class Z3950Client::Rep {
        public:
            boost::mutex m_mutex;
            boost::condition m_cond_session_ready;
            std::map<yp2::Session,Z3950Client::Assoc *> m_clients;
            Z3950Client::Assoc *get_assoc(Package &package);
            void send_and_receive(Package &package,
                                  yf::Z3950Client::Assoc *c);
            void release_assoc(Package &package);
        };
    }
}

using namespace yp2;

yf::Z3950Client::Assoc::Assoc(yazpp_1::SocketManager *socket_manager,
                              yazpp_1::IPDU_Observable *PDU_Observable,
                              std::string host)
    :  Z_Assoc(PDU_Observable),
       m_socket_manager(socket_manager), m_PDU_Observable(PDU_Observable),
       m_package(0), m_in_use(true), m_waiting(false), 
       m_destroyed(false), m_connected(false), m_host(host)
{
    // std::cout << "create assoc " << this << "\n";
}

yf::Z3950Client::Assoc::~Assoc()
{
    // std::cout << "destroy assoc " << this << "\n";
}

void yf::Z3950Client::Assoc::connectNotify()
{
    m_waiting = false;

    m_connected = true;
}

void yf::Z3950Client::Assoc::failNotify()
{
    m_waiting = false;

    yp2::odr odr;

    if (m_package)
    {
        m_package->response() = odr.create_close(Z_Close_peerAbort, 0);
        m_package->session().close();
    }
}

void yf::Z3950Client::Assoc::timeoutNotify()
{
    m_waiting = false;

#if 0
    yp2::odr odr;

    if (m_package)
    {
        m_package->response() = odr.create_close(Z_Close_lackOfActivity, 0);
        m_package->session().close();
    }
#endif
}

void yf::Z3950Client::Assoc::recv_GDU(Z_GDU *gdu, int len)
{
    m_waiting = false;

    if (m_package)
        m_package->response() = gdu;
}

yazpp_1::IPDU_Observer *yf::Z3950Client::Assoc::sessionNotify(
    yazpp_1::IPDU_Observable *the_PDU_Observable,
    int fd)
{
    return 0;
}


yf::Z3950Client::Z3950Client() :  m_p(new yf::Z3950Client::Rep)
{
}

yf::Z3950Client::~Z3950Client() {
}

yf::Z3950Client::Assoc *yf::Z3950Client::Rep::get_assoc(Package &package) 
{
    // only one thread messes with the clients list at a time
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<yp2::Session,yf::Z3950Client::Assoc *>::iterator it;
    
    Z_GDU *gdu = package.request().get();
    // only deal with Z39.50
    if (!gdu || gdu->which != Z_GDU_Z3950)
    {
        package.move();
        return 0;
    }
    Z_APDU *apdu = gdu->u.z3950;
    while(true)
    {
        it = m_clients.find(package.session());
        if (it == m_clients.end())
            break;
        if (gdu && gdu->which == Z_GDU_Z3950 &&
            gdu->u.z3950->which == Z_APDU_initRequest)
        {
            yazpp_1::SocketManager *s = it->second->m_socket_manager;
            delete it->second;  // destroy Z_Assoc
            delete s;    // then manager
            m_clients.erase(it);
            break;
        }
        if (!it->second->m_in_use)
        {
            it->second->m_in_use = true;
            return it->second;
        }
        m_cond_session_ready.wait(lock);
    }
    // new Z39.50 session ..

    // check that it is init. If not, close
    if (apdu->which != Z_APDU_initRequest)
    {
        yp2::odr odr;
        
        package.response() = odr.create_close(Z_Close_protocolError,
                                              "First PDU was not an "
                                              "Initialize Request");
        package.session().close();
        return 0;
    }
    // check virtual host
    const char *vhost =
        yaz_oi_get_string_oidval(&apdu->u.initRequest->otherInfo,
                                 VAL_PROXY, 1, 0);
    if (!vhost)
    {
        yp2::odr odr;
        package.response() = odr.create_initResponse(
            YAZ_BIB1_INIT_NEGOTIATION_OPTION_REQUIRED,
            "Virtual host not given");
        
        package.session().close();
        return 0;
    }
    
    yazpp_1::SocketManager *sm = new yazpp_1::SocketManager;
    yazpp_1::PDU_Assoc *pdu_as = new yazpp_1::PDU_Assoc(sm);
    yf::Z3950Client::Assoc *as = new yf::Z3950Client::Assoc(sm, pdu_as, vhost);
    m_clients[package.session()] = as;
    as->timeout(2);
    return as;
}

void yf::Z3950Client::Rep::send_and_receive(Package &package,
                                            yf::Z3950Client::Assoc *c)
{
    Z_GDU *gdu = package.request().get();

    if (!gdu || gdu->which != Z_GDU_Z3950)
        return;

    c->m_package = &package;
    c->m_waiting = true;
    if (!c->m_connected)
    {
        c->client(c->m_host.c_str());
        
        while (!c->m_destroyed && c->m_waiting 
               && c->m_socket_manager->processEvent() > 0)
            ;
    }
    if (!c->m_connected)
    {
        return;
    }

    // prepare response
    c->m_waiting = true;
    
    // relay the package  ..
    int len;
    c->send_GDU(gdu, &len);

    switch(gdu->u.z3950->which)
    {
    case Z_APDU_triggerResourceControlRequest:
        // request only..
        break;
    default:
        // for the rest: wait for a response PDU
        std::cout << "WAITING...\n";
        while (!c->m_destroyed && c->m_waiting
               && c->m_socket_manager->processEvent() > 0)
            ;
        std::cout << "END OF WAITING...\n";
        break;
    }
    
}

void yf::Z3950Client::Rep::release_assoc(Package &package)
{
    boost::mutex::scoped_lock lock(m_mutex);
    std::map<yp2::Session,yf::Z3950Client::Assoc *>::iterator it;
    
    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        if (package.session().is_closed())
            it->second->m_destroyed = true;
        
        if (it->second->m_destroyed && !it->second->m_in_use)
        {
            // the Z_Assoc and PDU_Assoc must be destroyed before
            // the socket manager.. so pull that out.. first..
            yazpp_1::SocketManager *s = it->second->m_socket_manager;
            delete it->second;  // destroy Z_Assoc
            delete s;    // then manager
            m_clients.erase(it);
            std::cout << "DESTROY " << package.session().id() << "\n";
        }
        else
        {
            it->second->m_in_use = false;
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
    }
    m_p->release_assoc(package);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
