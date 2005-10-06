/* $Id: thread_pool_observer.cpp,v 1.1 2005-10-06 19:33:58 adam Exp $
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
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

#include <yaz++/socket-observer.h>
#include <yaz/log.h>

#include "thread_pool_observer.h"

using namespace yazpp_1;

IThreadPoolMsg::~IThreadPoolMsg()
{

}

static void *tfunc(void *p)
{
    ThreadPoolSocketObserver *pt = (ThreadPoolSocketObserver *) p;
    pt->run(0);
    return 0;
}


ThreadPoolSocketObserver::ThreadPoolSocketObserver(ISocketObservable *obs, int no_threads)
    : m_SocketObservable(obs)
{
    pipe(m_fd);
    obs->addObserver(m_fd[0], this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);

    m_stop_flag = false;
    pthread_mutex_init(&m_mutex_input_data, 0);
    pthread_cond_init(&m_cond_input_data, 0);
    pthread_mutex_init(&m_mutex_output_data, 0);

    m_no_threads = no_threads;
    m_thread_id = new pthread_t[no_threads];
    int i;
    for (i = 0; i<m_no_threads; i++)
        pthread_create(&m_thread_id[i], 0, tfunc, this);
}

ThreadPoolSocketObserver::~ThreadPoolSocketObserver()
{
    pthread_mutex_lock(&m_mutex_input_data);
    m_stop_flag = true;
    pthread_cond_broadcast(&m_cond_input_data);
    pthread_mutex_unlock(&m_mutex_input_data);
    
    int i;
    for (i = 0; i<m_no_threads; i++)
        pthread_join(m_thread_id[i], 0);
    delete [] m_thread_id;

    m_SocketObservable->deleteObserver(this);

    pthread_cond_destroy(&m_cond_input_data);
    pthread_mutex_destroy(&m_mutex_input_data);
    pthread_mutex_destroy(&m_mutex_output_data);
    close(m_fd[0]);
    close(m_fd[1]);
}

void ThreadPoolSocketObserver::socketNotify(int event)
{
    if (event & SOCKET_OBSERVE_READ)
    {
        char buf[2];
        read(m_fd[0], buf, 1);
        pthread_mutex_lock(&m_mutex_output_data);
        IThreadPoolMsg *out = m_output.front();
        m_output.pop_front();
        pthread_mutex_unlock(&m_mutex_output_data);
        if (out)
            out->result();
    }
}

void ThreadPoolSocketObserver::run(void *p)
{
    while(1)
    {
        pthread_mutex_lock(&m_mutex_input_data);
        while (!m_stop_flag && m_input.size() == 0)
            pthread_cond_wait(&m_cond_input_data, &m_mutex_input_data);
        if (m_stop_flag)
        {
            pthread_mutex_unlock(&m_mutex_input_data);
            break;
        }
        IThreadPoolMsg *in = m_input.front();
        m_input.pop_front();
        pthread_mutex_unlock(&m_mutex_input_data);

        IThreadPoolMsg *out = in->handle();
        pthread_mutex_lock(&m_mutex_output_data);

        m_output.push_back(out);
        
        write(m_fd[1], "", 1);
        pthread_mutex_unlock(&m_mutex_output_data);
    }
}

void ThreadPoolSocketObserver::put(IThreadPoolMsg *m)
{
    pthread_mutex_lock(&m_mutex_input_data);
    m_input.push_back(m);
    pthread_cond_signal(&m_cond_input_data);
    pthread_mutex_unlock(&m_mutex_input_data);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

