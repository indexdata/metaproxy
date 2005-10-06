/* $Id: msg-thread.cpp,v 1.1 2005-10-06 09:37:25 marc Exp $
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

#include "msg-thread.h"

using namespace yazpp_1;

IMsg_Thread::~IMsg_Thread()
{

}

Msg_Thread_Queue::Msg_Thread_Queue()
{
    m_list = 0;
}

int Msg_Thread_Queue::size()
{
    int no = 0;
    Msg_Thread_Queue_List *l;
    for (l = m_list; l; l = l->m_next)
        no++;
    return no;
}

void Msg_Thread_Queue::enqueue(IMsg_Thread *m)
{
    Msg_Thread_Queue_List *l = new Msg_Thread_Queue_List;
    l->m_next = m_list;
    l->m_item = m;
    m_list = l;
}

IMsg_Thread *Msg_Thread_Queue::dequeue()
{
    Msg_Thread_Queue_List **l = &m_list;
    if (!*l)
        return 0;
    while ((*l)->m_next)
        l = &(*l)->m_next;
    IMsg_Thread *m = (*l)->m_item;
    delete *l;
    *l = 0;
    return m;
}

static void *tfunc(void *p)
{
    Msg_Thread *pt = (Msg_Thread *) p;
    pt->run(0);
    return 0;
}


Msg_Thread::Msg_Thread(ISocketObservable *obs, int no_threads)
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

Msg_Thread::~Msg_Thread()
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

void Msg_Thread::socketNotify(int event)
{
    if (event & SOCKET_OBSERVE_READ)
    {
        char buf[2];
        read(m_fd[0], buf, 1);
        pthread_mutex_lock(&m_mutex_output_data);
        IMsg_Thread *out = m_output.dequeue();
        pthread_mutex_unlock(&m_mutex_output_data);
        if (out)
            out->result();
    }
}

void Msg_Thread::run(void *p)
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
        IMsg_Thread *in = m_input.dequeue();
        pthread_mutex_unlock(&m_mutex_input_data);

        IMsg_Thread *out = in->handle();
        pthread_mutex_lock(&m_mutex_output_data);
        m_output.enqueue(out);
        
        write(m_fd[1], "", 1);
        pthread_mutex_unlock(&m_mutex_output_data);
    }
}

void Msg_Thread::put(IMsg_Thread *m)
{
    pthread_mutex_lock(&m_mutex_input_data);
    m_input.enqueue(m);
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

