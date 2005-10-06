/* $Id: test_thread_pool_observer.cpp,v 1.1 2005-10-06 19:33:58 adam Exp $
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

#include <stdlib.h>
#include <ctype.h>

#include <yaz++/pdu-assoc.h>
#include <yaz++/socket-manager.h>
#include <yaz/log.h>
#include "thread_pool_observer.h"

using namespace yazpp_1;

class My_Msg : public IThreadPoolMsg {
public:
    IThreadPoolMsg *handle();
    void result();
    int m_val;
};

IThreadPoolMsg *My_Msg::handle()
{
    My_Msg *res = new My_Msg;
    int sl = rand() % 5;

    res->m_val = m_val;
    printf("My_Msg::handle val=%d sleep=%d\n", m_val, sl);
    sleep(sl);
    return res;
}

void My_Msg::result()
{
    printf("My_Msg::result val=%d\n", m_val);
}

class My_Timer_Thread : public ISocketObserver {
private:
    ISocketObservable *m_obs;
    int m_fd[2];
    ThreadPoolSocketObserver *m_t;
public:
    My_Timer_Thread(ISocketObservable *obs, ThreadPoolSocketObserver *t);
    void socketNotify(int event);
};

My_Timer_Thread::My_Timer_Thread(ISocketObservable *obs,
                                 ThreadPoolSocketObserver *t) : m_obs(obs) 
{
    pipe(m_fd);
    m_t = t;
    obs->addObserver(m_fd[0], this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, 1);
}

void My_Timer_Thread::socketNotify(int event)
{
    static int seq = 1;
    printf("Add %d\n", seq);
    My_Msg *m = new My_Msg;
    m->m_val = seq++;
    m_t->put(m);
}

int main(int argc, char **argv)
{
    SocketManager mySocketManager;

    ThreadPoolSocketObserver m(&mySocketManager, 3);
    My_Timer_Thread t(&mySocketManager, &m) ;
    int i = 0;
    while (++i < 5 && mySocketManager.processEvent() > 0)
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

