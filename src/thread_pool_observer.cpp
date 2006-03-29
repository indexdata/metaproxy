
/* $Id: thread_pool_observer.cpp,v 1.15 2006-03-29 13:44:45 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */
#include "config.hpp"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <winsock.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <ctype.h>
#include <stdio.h>

#include <deque>

#include <yazpp/socket-observer.h>
#include <yaz/log.h>

#include "thread_pool_observer.hpp"
#include "pipe.hpp"

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    class ThreadPoolSocketObserver::Worker {
    public:
        Worker(ThreadPoolSocketObserver *s) : m_s(s) {};
        ThreadPoolSocketObserver *m_s;
        void operator() (void) {
            m_s->run(0);
        }
    };

    class ThreadPoolSocketObserver::Rep : public boost::noncopyable {
        friend class ThreadPoolSocketObserver;
    public:
        Rep(yazpp_1::ISocketObservable *obs);
        ~Rep();
    private:
        yazpp_1::ISocketObservable *m_socketObservable;
        Pipe m_pipe;
        boost::thread_group m_thrds;
        boost::mutex m_mutex_input_data;
        boost::condition m_cond_input_data;
        boost::mutex m_mutex_output_data;
        std::deque<IThreadPoolMsg *> m_input;
        std::deque<IThreadPoolMsg *> m_output;
        bool m_stop_flag;
        int m_no_threads;
    };
}


using namespace yazpp_1;
using namespace mp;

ThreadPoolSocketObserver::Rep::Rep(ISocketObservable *obs)
    : m_socketObservable(obs), m_pipe(9123)
{
}

ThreadPoolSocketObserver::Rep::~Rep()
{
}

IThreadPoolMsg::~IThreadPoolMsg()
{

}

ThreadPoolSocketObserver::ThreadPoolSocketObserver(ISocketObservable *obs,
                                                   int no_threads)
    : m_p(new Rep(obs))
{
    obs->addObserver(m_p->m_pipe.read_fd(), this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);

    m_p->m_stop_flag = false;
    m_p->m_no_threads = no_threads;
    int i;
    for (i = 0; i<no_threads; i++)
    {
        Worker w(this);
        m_p->m_thrds.add_thread(new boost::thread(w));
    }
}

ThreadPoolSocketObserver::~ThreadPoolSocketObserver()
{
    {
        boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);
        m_p->m_stop_flag = true;
        m_p->m_cond_input_data.notify_all();
    }
    m_p->m_thrds.join_all();

    m_p->m_socketObservable->deleteObserver(this);
}

void ThreadPoolSocketObserver::socketNotify(int event)
{
    if (event & SOCKET_OBSERVE_READ)
    {
        char buf[2];
        recv(m_p->m_pipe.read_fd(), buf, 1, 0);
        IThreadPoolMsg *out;
        {
            boost::mutex::scoped_lock output_lock(m_p->m_mutex_output_data);
            out = m_p->m_output.front();
            m_p->m_output.pop_front();
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
            boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);
            while (!m_p->m_stop_flag && m_p->m_input.size() == 0)
                m_p->m_cond_input_data.wait(input_lock);
            if (m_p->m_stop_flag)
                break;
            
            in = m_p->m_input.front();
            m_p->m_input.pop_front();
        }
        IThreadPoolMsg *out = in->handle();
        {
            boost::mutex::scoped_lock output_lock(m_p->m_mutex_output_data);
            m_p->m_output.push_back(out);
            send(m_p->m_pipe.write_fd(), "", 1, 0);
        }
    }
}

void ThreadPoolSocketObserver::put(IThreadPoolMsg *m)
{
    boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);
    m_p->m_input.push_back(m);
    m_p->m_cond_input_data.notify_one();
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

