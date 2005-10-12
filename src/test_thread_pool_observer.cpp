/* $Id: test_thread_pool_observer.cpp,v 1.3 2005-10-12 23:30:43 adam Exp $
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

#include "config.hpp"
#include <stdlib.h>
#include <ctype.h>

#include <yaz++/pdu-assoc.h>
#include <yaz++/socket-manager.h>
#include <yaz/log.h>
#include "thread_pool_observer.h"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
using namespace yazpp_1;

class My_Timer_Thread;

class My_Msg : public IThreadPoolMsg {
public:
    IThreadPoolMsg *handle();
    void result();
    int m_val;
    My_Timer_Thread *m_timer;
};

class My_Timer_Thread : public ISocketObserver {
private:
    ISocketObservable *m_obs;
    int m_fd[2];
    ThreadPoolSocketObserver *m_t;
public:
    int m_sum;
    int m_requests;
    int m_responses;
    My_Timer_Thread(ISocketObservable *obs, ThreadPoolSocketObserver *t);
    void socketNotify(int event);
};


IThreadPoolMsg *My_Msg::handle()
{
    My_Msg *res = new My_Msg;

    if (m_val == 7)
        sleep(1);

    res->m_val = m_val;
    res->m_timer = m_timer;
    return res;
}

void My_Msg::result()
{
    m_timer->m_sum += m_val;
    m_timer->m_responses++;
}

My_Timer_Thread::My_Timer_Thread(ISocketObservable *obs,
                                 ThreadPoolSocketObserver *t) : m_obs(obs) 
{
    pipe(m_fd);
    m_t = t;
    m_sum = 0;
    m_requests = 0;
    m_responses = 0;
    obs->addObserver(m_fd[0], this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, 0);
}

void My_Timer_Thread::socketNotify(int event)
{
    My_Msg *m = new My_Msg;
    m->m_val = m_requests++;
    m->m_timer = this;
    m_t->put(m);
}

BOOST_AUTO_TEST_CASE( thread_pool_observer1 ) 
{
    SocketManager mySocketManager;

    ThreadPoolSocketObserver m(&mySocketManager, 3);
    My_Timer_Thread t(&mySocketManager, &m) ;
    while (t.m_responses < 30 && mySocketManager.processEvent() > 0)
        ;
    BOOST_CHECK_EQUAL(t.m_responses, 30);
    BOOST_CHECK(t.m_sum >= 435);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

