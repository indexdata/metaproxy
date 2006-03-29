/* $Id: filter_z3950_client.cpp,v 1.25 2006-03-29 13:44:45 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
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
                  std::string host, int timeout);
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
            int m_queue_len;
            int m_time_elapsed;
            int m_time_max;
            std::string m_host;
        };

        class Z3950Client::Rep {
        public:
            int m_timeout_sec;
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
                              std::string host, int timeout_sec)
    :  Z_Assoc(PDU_Observable),
       m_socket_manager(socket_manager), m_PDU_Observable(PDU_Observable),
       m_package(0), m_in_use(true), m_waiting(false), 
       m_destroyed(false), m_connected(false), m_queue_len(1),
       m_time_elapsed(0), m_time_max(timeout_sec), 
       m_host(host)
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
    if (m_time_elapsed >= m_time_max)
    {
        m_waiting = false;

        mp::odr odr;
        
        if (m_package)
        {
            Z_GDU *gdu = m_package->request().get();
            Z_APDU *apdu = 0;
            if (gdu && gdu->which == Z_GDU_Z3950)
                apdu = gdu->u.z3950;
        
            if (m_connected)
                m_package->response() =
                    odr.create_close(apdu, Z_Close_lackOfActivity, 0);
            else
                m_package->response() = 
                    odr.create_close(apdu, Z_Close_peerAbort, 0);
                
            m_package->session().close();
        }
    }
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
    m_p->m_timeout_sec = 30;
}

yf::Z3950Client::~Z3950Client() {
}

yf::Z3950Client::Assoc *yf::Z3950Client::Rep::get_assoc(Package &package) 
{
    // only one thread messes with the clients list at a time
    boost::mutex::scoped_lock lock(m_mutex);

    std::map<mp::Session,yf::Z3950Client::Assoc *>::iterator it;
    
    Z_GDU *gdu = package.request().get();
    // only deal with Z39.50
    if (!gdu || gdu->which != Z_GDU_Z3950)
    {
        package.move();
        return 0;
    }
    it = m_clients.find(package.session());
    if (it != m_clients.end())
    {
        it->second->m_queue_len++;
        while(true)
        {
#if 0
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
    std::list<std::string> vhosts;
    mp::util::get_vhost_otherinfo(&apdu->u.initRequest->otherInfo,
                                   true, vhosts);
    size_t no_vhosts = vhosts.size();
    if (no_vhosts == 0)
    {
        mp::odr odr;
        package.response() = odr.create_initResponse(
            apdu,
            YAZ_BIB1_INIT_NEGOTIATION_OPTION_REQUIRED,
            "z3950_client: No virtal host given");
        
        package.session().close();
        return 0;
    }
    if (no_vhosts > 1)
    {
        mp::odr odr;
        package.response() = odr.create_initResponse(
            apdu,
            YAZ_BIB1_COMBI_OF_SPECIFIED_DATABASES_UNSUPP,
            "z3950_client: Can not cope with multiple vhosts");
        package.session().close();
        return 0;
    }
    std::list<std::string>::const_iterator v_it = vhosts.begin();
    std::list<std::string> dblist;
    std::string host;
    mp::util::split_zurl(*v_it, host, dblist);
    
    if (dblist.size())
    {
        ; // z3950_client: Databases in vhost ignored
    }

    yazpp_1::SocketManager *sm = new yazpp_1::SocketManager;
    yazpp_1::PDU_Assoc *pdu_as = new yazpp_1::PDU_Assoc(sm);
    yf::Z3950Client::Assoc *as = new yf::Z3950Client::Assoc(sm, pdu_as,
                                                            host.c_str(),
                                                            m_timeout_sec);
    m_clients[package.session()] = as;
    return as;
}

void yf::Z3950Client::Rep::send_and_receive(Package &package,
                                            yf::Z3950Client::Assoc *c)
{
    Z_GDU *gdu = package.request().get();

    if (c->m_destroyed)
        return;

    if (!gdu || gdu->which != Z_GDU_Z3950)
        return;

    c->m_time_elapsed = 0;
    c->m_package = &package;
    c->m_waiting = true;
    if (!c->m_connected)
    {
        c->client(c->m_host.c_str());
        c->timeout(1);

        while (!c->m_destroyed && c->m_waiting 
               && c->m_socket_manager->processEvent() > 0)
            ;
    }
    if (!c->m_connected)
    {
        return;
    }

    // prepare response
    c->m_time_elapsed = 0;
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
        Z_GDU *gdu = package.request().get();
        if (gdu && gdu->which == Z_GDU_Z3950)
        {   // only Z39.50 packages lock in get_assoc.. release it
            it->second->m_in_use = false;
            it->second->m_queue_len--;
        }

        if (package.session().is_closed())
        {
            // destroy hint (send_and_receive)
            it->second->m_destroyed = true;
            
            // wait until no one is waiting for it.
            while (it->second->m_queue_len)
                m_cond_session_ready.wait(lock);
            
            // the Z_Assoc and PDU_Assoc must be destroyed before
            // the socket manager.. so pull that out.. first..
            yazpp_1::SocketManager *s = it->second->m_socket_manager;
            delete it->second;  // destroy Z_Assoc
            delete s;    // then manager
            m_clients.erase(it);
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

void yf::Z3950Client::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "timeout"))
        {
            std::string timeout_str = mp::xml::get_text(ptr);
            int timeout_sec = atoi(timeout_str.c_str());
            if (timeout_sec < 2)
                throw mp::filter::FilterException("Bad timeout value " 
                                                   + timeout_str);
            m_p->m_timeout_sec = timeout_sec;
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
