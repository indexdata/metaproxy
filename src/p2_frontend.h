/* $Id: p2_frontend.h,v 1.1.1.1 2005-10-06 09:37:25 marc Exp $
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

#ifndef P2_FRONTEND_H
#define P2_FRONTEND_H

#include <list>
#include <vector>
#include <string>

#include "msg-thread.h"
#include <yaz++/z-assoc.h>
#include <yaz++/pdu-assoc.h>
#include <yaz++/gdu.h>
#include <yaz++/z-query.h>

class P2_Frontend;
class P2_Server;
class P2_Config;
class P2_ConfigTarget;
class P2_ModuleFactory;

class IP2_BackendSet;

class P2_BackendResultSet {
 public:
    P2_BackendResultSet();
    ~P2_BackendResultSet();
    yazpp_1::Yaz_Z_Query m_query;
    std::list<std::string> m_db_list;
    int m_hit_count;
    IP2_BackendSet *m_int;
    // record cache here 
};

class IP2_Backend;

class P2_Backend {
 public:
    P2_Backend(P2_ConfigTarget *cfg, IP2_Backend *backend_interface);
    ~P2_Backend();
 public:
    std::list<P2_BackendResultSet *>m_resultSets;
    P2_ConfigTarget *m_configTarget;
    bool m_busy;
    IP2_Backend *m_int;
};

class P2_Server : public yazpp_1::Z_Assoc {
public:
    ~P2_Server();
    P2_Server(yazpp_1::IPDU_Observable *the_PDU_Observable,
              Msg_Thread *m_my_thread,
              P2_Config *config,
              P2_ModuleFactory *modules);
    P2_Config *lockConfig();
    void unlockConfig();
    std::list<P2_Backend *>m_backend_list;
    P2_ModuleFactory *m_modules;
private:
    yazpp_1::IPDU_Observer* sessionNotify(
        yazpp_1::IPDU_Observable *the_PDU_Observable,
        int fd);
    void recv_GDU(Z_GDU *apdu, int len);

    void failNotify();
    void timeoutNotify();
    void connectNotify();
private:
    P2_Config *m_config;
    Msg_Thread *m_my_thread;
    pthread_mutex_t m_mutex_config;
};

class P2_FrontResultSet {
public:
    P2_FrontResultSet(const char *id);
    ~P2_FrontResultSet();
    void setQuery(Z_Query *z_query);
    void setDatabases(char **db, int num);
    std::string m_resultSetId;
    std::vector<std::string> m_db_list;
    yazpp_1::Yaz_Z_Query m_query;
};

class P2_Msg : public IMsg_Thread {
public:
    int m_close_flag;
    yazpp_1::GDU *m_gdu;
    yazpp_1::GDU *m_output;
    P2_Frontend *m_front;
    P2_Server *m_server;
    IMsg_Thread *handle();
    void result();
    P2_Msg(yazpp_1::GDU *gdu, P2_Frontend *front, P2_Server *server);
    virtual ~P2_Msg();
 private:

    Z_APDU *frontend_search_resultset(Z_APDU *z_gdu, ODR odr,
                                      P2_FrontResultSet **rset);
    Z_APDU *frontend_present_resultset(Z_APDU *z_gdu, ODR odr,
                                       P2_FrontResultSet **rset);
    Z_APDU *frontend_search_apdu(Z_APDU *z_gdu, ODR odr);
    Z_APDU *frontend_present_apdu(Z_APDU *z_gdu, ODR odr);
    P2_Backend *select_backend(std::string db,
                               yazpp_1::Yaz_Z_Query *query,
                               P2_BackendResultSet **bset);
    P2_Backend *create_backend(std::string db);
};

class P2_Frontend : public yazpp_1::Z_Assoc {
 public:
    ~P2_Frontend();
    P2_Frontend(yazpp_1::IPDU_Observable *the_PDU_Observable,
                Msg_Thread *m_my_thread, P2_Server *server);
    IPDU_Observer* sessionNotify(yazpp_1::IPDU_Observable *the_PDU_Observable,
                                 int fd);
    
    void recv_GDU(Z_GDU *apdu, int len);
    
    void failNotify();
    void timeoutNotify();
    void connectNotify();

    int m_no_requests;
    int m_delete_flag;
    std::list<P2_FrontResultSet *> m_resultSets;
    
 private:
    yazpp_1::GDUQueue m_in_queue;
    Msg_Thread *m_my_thread;
    P2_Server *m_server;
 private:
    bool P2_Frontend::search(Z_GDU *z_gdu);
    bool P2_Frontend::handle_init(Z_GDU *z_gdu);
};

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
