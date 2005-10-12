/* $Id: thread_pool_observer.cpp,v 1.3 2005-10-12 23:30:43 adam Exp $
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
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

#include <yaz++/socket-observer.h>
#include <yaz/log.h>

#include "config.hpp"
#include "thread_pool_observer.h"

using namespace yazpp_1;

IThreadPoolMsg::~IThreadPoolMsg()
{

}

class worker {
public:
    worker(ThreadPoolSocketObserver *s) : m_s(s) {};
    ThreadPoolSocketObserver *m_s;
    void operator() (void) {
        m_s->run(0);
    }
};

ThreadPoolSocketObserver::ThreadPoolSocketObserver(ISocketObservable *obs, int no_threads)
    : m_SocketObservable(obs)
{
    pipe(m_fd);
    obs->addObserver(m_fd[0], this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);

    m_stop_flag = false;
    m_no_threads = no_threads;
    int i;
    for (i = 0; i<no_threads; i++)
    {
        worker w(this);
        m_thrds.add_thread(new boost::thread(w));
    }
}

ThreadPoolSocketObserver::~ThreadPoolSocketObserver()
{
    {
        boost::mutex::scoped_lock input_lock(m_mutex_input_data);
        m_stop_flag = true;
        m_cond_input_data.notify_all();
    }
    m_thrds.join_all();

    m_SocketObservable->deleteObserver(this);

    close(m_fd[0]);
    close(m_fd[1]);
}

void ThreadPoolSocketObserver::socketNotify(int event)
{
    if (event & SOCKET_OBSERVE_READ)
    {
        char buf[2];
        read(m_fd[0], buf, 1);
        IThreadPoolMsg *out;
        {
            boost::mutex::scoped_lock output_lock(m_mutex_output_data);
            out = m_output.front();
            m_output.pop_front();
        }
        if (out)
            out->result();
    }
}

void ThreadPoolSocketObserver::run(void *p)
{
    while(1)
    {
        IThreadPoolMsg *in = 0;
        {
            boost::mutex::scoped_lock input_lock(m_mutex_input_data);
            while (!m_stop_flag && m_input.size() == 0)
                m_cond_input_data.wait(input_lock);
            if (m_stop_flag)
                break;
            
            in = m_input.front();
            m_input.pop_front();
        }
        IThreadPoolMsg *out = in->handle();
        {
            boost::mutex::scoped_lock output_lock(m_mutex_output_data);
            m_output.push_back(out);
            write(m_fd[1], "", 1);
        }
    }
}

void ThreadPoolSocketObserver::put(IThreadPoolMsg *m)
{
    boost::mutex::scoped_lock input_lock(m_mutex_input_data);
    m_input.push_back(m);
    m_cond_input_data.notify_one();
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

