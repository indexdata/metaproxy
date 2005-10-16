/* $Id: filter_z3950_client.cpp,v 1.1 2005-10-16 16:05:44 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "filter_z3950_client.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>
#include <yaz/otherinfo.h>
#include <yaz/diagbib1.h>

#include <yaz++/socket-manager.h>
#include <yaz++/pdu-assoc.h>
#include <yaz++/z-assoc.h>

#include <iostream>

namespace yf = yp2::filter;

namespace yp2 {
    namespace filter {
        class Z3950Client::Assoc : public yazpp_1::Z_Assoc{
            friend class Pimpl;
        public:
            Assoc(yp2::Session id, yazpp_1::SocketManager *socket_manager,
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
        private:
            yp2::Session m_session_id;
            yazpp_1::SocketManager *m_socket_manager;
            yazpp_1::IPDU_Observable *m_PDU_Observable;
            Package *m_package;
            bool m_waiting;
            bool m_connected;
            std::string m_host;
        };

        class Z3950Client::Pimpl {
        public:
            boost::mutex m_mutex;
            std::list<Z3950Client::Assoc *> m_clients;
            Z3950Client::Assoc *get_assoc(Package &package);
            void send_and_receive(Package &package,
                                  yf::Z3950Client::Assoc *c);
        };
    }
}


yf::Z3950Client::Assoc::Assoc(yp2::Session id,
                              yazpp_1::SocketManager *socket_manager,
                              yazpp_1::IPDU_Observable *PDU_Observable,
                              std::string host)
    :  Z_Assoc(PDU_Observable), m_session_id(id),
       m_socket_manager(socket_manager), m_PDU_Observable(PDU_Observable),
       m_package(0), m_waiting(false), m_connected(false),
       m_host(host)
{
}

yf::Z3950Client::Assoc::~Assoc()
{
    delete m_socket_manager;
}

void yf::Z3950Client::Assoc::connectNotify()
{
    m_waiting = false;

    m_connected = true;
}

void yf::Z3950Client::Assoc::failNotify()
{
    m_waiting = false;

    ODR odr = odr_createmem(ODR_ENCODE);

    Z_APDU *apdu = zget_APDU(odr, Z_APDU_close);

    *apdu->u.close->closeReason = Z_Close_peerAbort;

    if (m_package)
    {
        m_package->response() = apdu;
        m_package->session().close();
    }

    odr_destroy(odr);
}

void yf::Z3950Client::Assoc::timeoutNotify()
{
    m_waiting = false;

    ODR odr = odr_createmem(ODR_ENCODE);

    Z_APDU *apdu = zget_APDU(odr, Z_APDU_close);

    *apdu->u.close->closeReason = Z_Close_lackOfActivity;

    if (m_package)
    {
        m_package->response() = apdu;
        m_package->session().close();
    }
    odr_destroy(odr);
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


yf::Z3950Client::Z3950Client() {
    m_p = new yf::Z3950Client::Pimpl;
}

yf::Z3950Client::~Z3950Client() {
    delete m_p;
}

yf::Z3950Client::Assoc *yf::Z3950Client::Pimpl::get_assoc(Package &package) 
{
    Z_GDU *gdu = package.request().get();

    // only deal with Z39.50
    if (!gdu || gdu->which != Z_GDU_Z3950)
    {
        package.move();
        return 0;
    }

    // only one thread messes with the clients list at a time
    boost::mutex::scoped_lock lock(m_mutex);

    Z_APDU *apdu = gdu->u.z3950;

    std::list<yf::Z3950Client::Assoc *>::iterator it;

    for (it = m_clients.begin(); it != m_clients.end(); it++)
    {
        if ((*it)->m_session_id == package.session())
            break;
    }
    if (it != m_clients.end())
        return *it;

    // new session ..

    // check that it is init. If not, close
    if (apdu->which != Z_APDU_initRequest)
    {
        ODR odr = odr_createmem(ODR_ENCODE);
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_close);
        
        *apdu->u.close->closeReason = Z_Close_protocolError;
        package.response() = apdu;
        
        package.session().close();
        odr_destroy(odr);
        return 0;
    }
    // check virtual host
    const char *vhost =
        yaz_oi_get_string_oidval(&apdu->u.initRequest->otherInfo,
                                 VAL_PROXY, 1, 0);
    if (!vhost)
    {
        ODR odr = odr_createmem(ODR_ENCODE);
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
        
        apdu->u.initResponse->userInformationField =
            zget_init_diagnostics(odr, 
                                  YAZ_BIB1_INIT_NEGOTIATION_OPTION_REQUIRED,
                                  "Virtual host not given");
        package.response() = apdu;
            
        package.session().close();
        odr_destroy(odr);
        return 0;
    }
    
    yazpp_1::SocketManager *sm = new yazpp_1::SocketManager;
    yazpp_1::PDU_Assoc *pdu_as = new yazpp_1::PDU_Assoc(sm);
    yf::Z3950Client::Assoc *as = new yf::Z3950Client::Assoc(package.session(),
                                                            sm, pdu_as,
                                                            vhost);
    m_clients.push_back(as);
    return as;
}

void yf::Z3950Client::Pimpl::send_and_receive(Package &package,
                                              yf::Z3950Client::Assoc *c)
{
    // we should lock c!

    c->m_package = &package;
    c->m_waiting = true;
    if (!c->m_connected)
    {
        c->client(c->m_host.c_str());

        while (c->m_waiting && c->m_socket_manager->processEvent() > 0)
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
    c->send_GDU(package.request().get(), &len);
    
    while (c->m_waiting && c->m_socket_manager->processEvent() > 0)
        ;
}

void yf::Z3950Client::process(Package &package) const
{
    yf::Z3950Client::Assoc *c = m_p->get_assoc(package);
    if (!c)
        return;
    m_p->send_and_receive(package, c);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
