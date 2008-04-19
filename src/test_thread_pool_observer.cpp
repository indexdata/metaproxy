/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

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
#include <stdlib.h>
#include <ctype.h>

#include <yazpp/pdu-assoc.h>
#include <yazpp/socket-manager.h>
#include <yaz/log.h>
#include "pipe.hpp"
#include "thread_pool_observer.hpp"

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
using namespace yazpp_1;
namespace mp = metaproxy_1;

class My_Timer_Thread;

class My_Msg : public mp::IThreadPoolMsg {
public:
    mp::IThreadPoolMsg *handle();
    void result();
    int m_val;
    My_Timer_Thread *m_timer;
};

class My_Timer_Thread : public ISocketObserver {
private:
    ISocketObservable *m_obs;
    mp::Pipe m_pipe;
    mp::ThreadPoolSocketObserver *m_t;
public:
    int m_sum;
    int m_requests;
    int m_responses;
    My_Timer_Thread(ISocketObservable *obs, mp::ThreadPoolSocketObserver *t);
    void socketNotify(int event);
};


mp::IThreadPoolMsg *My_Msg::handle()
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
                                 mp::ThreadPoolSocketObserver *t) : 
    m_obs(obs), m_pipe(9123) 
{
    m_t = t;
    m_sum = 0;
    m_requests = 0;
    m_responses = 0;
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, 0);
}

void My_Timer_Thread::socketNotify(int event)
{
    My_Msg *m = new My_Msg;
    m->m_val = m_requests++;
    m->m_timer = this;
    m_t->put(m);
#if 0
    // prevent input queue from being filled up..
    // bug #1064: Test test_thread_pool_observer hangs
    // fortunately we don't need this hack. because put (ebove)
    // will block itself if needed
    if (m->m_val == 30)
         m_obs->deleteObserver(this);
#endif
}

BOOST_AUTO_TEST_CASE( thread_pool_observer1 ) 
{
    SocketManager mySocketManager;

    mp::ThreadPoolSocketObserver m(&mySocketManager, 3);
    My_Timer_Thread t(&mySocketManager, &m) ;
    while (t.m_responses < 30 && mySocketManager.processEvent() > 0)
        ;
    BOOST_CHECK_EQUAL(t.m_responses, 30);
    BOOST_CHECK(t.m_sum >= 435); // = 29*30/2
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

