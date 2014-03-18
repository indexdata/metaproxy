/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#endif
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <ctype.h>
#include <stdio.h>

#include <deque>
#include <sstream>

#include <yazpp/socket-observer.h>
#include <yaz/log.h>

#include "thread_pool_observer.hpp"
#include "pipe.hpp"

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
        boost::condition m_cond_input_full;
        boost::mutex m_mutex_output_data;
        std::deque<IThreadPoolMsg *> m_input;
        std::deque<IThreadPoolMsg *> m_output;
        bool m_stop_flag;
        unsigned m_no_threads;
        unsigned m_no_threads_waiting;
    };
    const unsigned int queue_size_per_thread = 64;
}



using namespace yazpp_1;
using namespace metaproxy_1;

ThreadPoolSocketObserver::Rep::Rep(yazpp_1::ISocketObservable *obs)
    : m_socketObservable(obs), m_pipe(9123)
{
}

ThreadPoolSocketObserver::Rep::~Rep()
{
}

IThreadPoolMsg::~IThreadPoolMsg()
{

}

ThreadPoolSocketObserver::ThreadPoolSocketObserver(
    yazpp_1::ISocketObservable *obs, int no_threads)
    : m_p(new Rep(obs))
{
    obs->addObserver(m_p->m_pipe.read_fd(), this);
    obs->maskObserver(this, SOCKET_OBSERVE_READ);

    m_p->m_stop_flag = false;
    m_p->m_no_threads = no_threads;
    m_p->m_no_threads_waiting = 0;
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
#ifdef WIN32
        recv(m_p->m_pipe.read_fd(), buf, 1, 0);
#else
        ssize_t r = read(m_p->m_pipe.read_fd(), buf, 1);
        if (r != 1)
        {
            if (r == (ssize_t) (-1))
                yaz_log(YLOG_WARN|YLOG_ERRNO,
                        "ThreadPoolSocketObserver::socketNotify. read fail");
            else
                yaz_log(YLOG_WARN,
                        "ThreadPoolSocketObserver::socketNotify. read returned 0");
        }
#endif
        IThreadPoolMsg *out;
        {
            boost::mutex::scoped_lock output_lock(m_p->m_mutex_output_data);
            out = m_p->m_output.front();
            m_p->m_output.pop_front();
        }


        if (out)
        {
            std::ostringstream os;
            {
                boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);
                os  << "tbusy/total " <<
                    m_p->m_no_threads - m_p->m_no_threads_waiting <<
                    "/" << m_p->m_no_threads
                    << " queue in/out " << m_p->m_input.size() << "/"
                    << m_p->m_output.size();
            }
            out->result(os.str().c_str());
        }
    }
}

void ThreadPoolSocketObserver::get_thread_info(int &tbusy, int &total)
{
    tbusy = m_p->m_no_threads - m_p->m_no_threads_waiting;
    total = m_p->m_no_threads;
}

void ThreadPoolSocketObserver::run(void *p)
{
    while(1)
    {
        IThreadPoolMsg *in = 0;
        {
            boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);
            m_p->m_no_threads_waiting++;
            while (!m_p->m_stop_flag && m_p->m_input.size() == 0)
                m_p->m_cond_input_data.wait(input_lock);
            m_p->m_no_threads_waiting--;
            if (m_p->m_stop_flag)
                break;

            in = m_p->m_input.front();
            m_p->m_input.pop_front();
            m_p->m_cond_input_full.notify_all();
        }
        IThreadPoolMsg *out = in->handle();
        {
            boost::mutex::scoped_lock output_lock(m_p->m_mutex_output_data);
            m_p->m_output.push_back(out);
#ifdef WIN32
            send(m_p->m_pipe.write_fd(), "", 1, 0);
#else
            ssize_t r = write(m_p->m_pipe.write_fd(), "", 1);
            if (r != 1)
            {
                if (r == (ssize_t) (-1))
                    yaz_log(YLOG_WARN|YLOG_ERRNO,
                            "ThreadPoolSocketObserver::run. write fail");
                else
                    yaz_log(YLOG_WARN,
                            "ThreadPoolSocketObserver::run. write returned 0");
            }
#endif
        }
    }
}

void ThreadPoolSocketObserver::cleanup(IThreadPoolMsg *m, void *info)
{
    boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);

    std::deque<IThreadPoolMsg *>::iterator it = m_p->m_input.begin();
    while (it != m_p->m_input.end())
    {
        if ((*it)->cleanup(info))
        {
            delete *it;
            it = m_p->m_input.erase(it);
        }
        else
            it++;
    }
}

void ThreadPoolSocketObserver::put(IThreadPoolMsg *m)
{
    boost::mutex::scoped_lock input_lock(m_p->m_mutex_input_data);

    while (m_p->m_input.size() >= m_p->m_no_threads * queue_size_per_thread)
        m_p->m_cond_input_full.wait(input_lock);
    m_p->m_input.push_back(m);
    m_p->m_cond_input_data.notify_one();
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

