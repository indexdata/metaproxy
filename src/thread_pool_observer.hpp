/* $Id: thread_pool_observer.hpp,v 1.4 2005-10-15 14:09:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef YP2_THREAD_POOL_OBSERVER_HPP
#define YP2_THREAD_POOL_OBSERVER_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <unistd.h>
#include <ctype.h>

#include <deque>
#include <yaz++/socket-observer.h>
#include <yaz/yconfig.h>

namespace yp2 {
    class IThreadPoolMsg {
    public:
        virtual IThreadPoolMsg *handle() = 0;
        virtual void result() = 0;
        virtual ~IThreadPoolMsg();
    };

    class ThreadPoolSocketObserver : public yazpp_1::ISocketObserver {
    private:
        class Worker {
        public:
            Worker(ThreadPoolSocketObserver *s) : m_s(s) {};
            ThreadPoolSocketObserver *m_s;
            void operator() (void) {
                m_s->run(0);
            }
        };
    public:
        ThreadPoolSocketObserver(yazpp_1::ISocketObservable *obs,
                                 int no_threads);
        virtual ~ThreadPoolSocketObserver();
        void socketNotify(int event);
        void put(IThreadPoolMsg *m);
        IThreadPoolMsg *get();
        void run(void *p);
        int m_fd[2];
    private:
        yazpp_1::ISocketObservable *m_SocketObservable;
        int m_no_threads;
        boost::thread_group m_thrds;

        std::deque<IThreadPoolMsg *> m_input;
        std::deque<IThreadPoolMsg *> m_output;

        boost::mutex m_mutex_input_data;
        boost::condition m_cond_input_data;
        boost::mutex m_mutex_output_data;
        bool m_stop_flag;

    
    };
}
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

