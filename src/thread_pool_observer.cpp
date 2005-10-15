/* $Id: thread_pool_observer.cpp,v 1.7 2005-10-15 14:09:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

#include <yaz++/socket-observer.h>
#include <yaz/log.h>

#include "config.hpp"
#include "thread_pool_observer.hpp"

using namespace yazpp_1;
using namespace yp2;

IThreadPoolMsg::~IThreadPoolMsg()
{

}

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
        Worker w(this);
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
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

