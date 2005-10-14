/* $Id: p2.cpp,v 1.4 2005-10-14 10:27:18 adam Exp $
   Copyright (c) 1998-2005, Index Data.

This file is part of the yaz-proxy.

YAZ proxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

YAZ proxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with YAZ proxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include <pthread.h>
#include <stdlib.h>
#include <yaz/log.h>
#include <yaz/diagbib1.h>
#include <yaz/options.h>

#include "config.hpp"
#include <yaz++/socket-manager.h>
#include "p2_config.h"
#include "p2_frontend.h"
#include "p2_xmlerror.h"
#include "p2_modules.h"

using namespace yazpp_1;

extern P2_ModuleEntry *p2_backend_dummy;

/*
  frontend result set map
    resultset -> db,query

  backend result set map
    db,query -> resultset, target
                resultset, target
*/
class P2_Frontend;

P2_Config *P2_Server::lockConfig()
{
    pthread_mutex_lock(&m_mutex_config);
    return m_config;
}

void P2_Server::unlockConfig()
{
    pthread_mutex_unlock(&m_mutex_config);
}

P2_Server::P2_Server(IPDU_Observable *the_PDU_Observable,
                     yp2::ThreadPoolSocketObserver *my_thread,
                     P2_Config *config,
                     P2_ModuleFactory *modules)
    :  Z_Assoc(the_PDU_Observable)
{
    m_my_thread = my_thread;
    m_modules = modules;
    m_config = config;

    pthread_mutex_init(&m_mutex_config, 0);
    
    yaz_log(YLOG_LOG, "Construct P2_Server=%p", this);
}

IPDU_Observer *P2_Server::sessionNotify(IPDU_Observable
                                       *the_PDU_Observable, int fd)
{
    P2_Frontend *my = new P2_Frontend(the_PDU_Observable, m_my_thread, this);
    yaz_log(YLOG_LOG, "New session %s", the_PDU_Observable->getpeername());
    return my;
}

P2_Server::~P2_Server()
{
    yaz_log(YLOG_LOG, "Destroy P2_server=%p", this);
    pthread_mutex_destroy(&m_mutex_config);
}

void P2_Server::recv_GDU(Z_GDU *apdu, int len)
{
}

void P2_Server::failNotify()
{
}

void P2_Server::timeoutNotify()
{
}

void P2_Server::connectNotify()
{
}

int main(int argc, char **argv)
{
    p2_xmlerror_setup();

    P2_Config config;

    if (!config.parse_options(argc, argv))
    {
        yaz_log(YLOG_FATAL, "Configuration incorrect. Exiting");
        exit(1);
    }

    SocketManager mySocketManager;

    PDU_Assoc *my_PDU_Assoc = 0;
    
    yp2::ThreadPoolSocketObserver my_thread(&mySocketManager,
                                            config.m_no_threads);

    my_PDU_Assoc = new PDU_Assoc(&mySocketManager);

    P2_ModuleFactory modules;

    modules.add(p2_backend_dummy);

    std::list<P2_ConfigModule *>::const_iterator it;
    for (it = config.m_modules.begin(); it != config.m_modules.end(); it++)
        modules.add((*it)->m_fname.c_str());
    
    P2_Server z(my_PDU_Assoc, &my_thread, &config, &modules);
    z.server(config.m_listen_address.c_str());

    while (mySocketManager.processEvent() > 0)
        ;
    return 0;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

