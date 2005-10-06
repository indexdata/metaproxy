/* $Id: thread_pool_observer.h,v 1.1 2005-10-06 19:33:58 adam Exp $
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

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <deque>
#include <yaz++/socket-observer.h>
#include <yaz/yconfig.h>

class IThreadPoolMsg {
public:
    virtual IThreadPoolMsg *handle() = 0;
    virtual void result() = 0;
    virtual ~IThreadPoolMsg();
};

class ThreadPoolSocketObserver : public yazpp_1::ISocketObserver {
 public:
    ThreadPoolSocketObserver(yazpp_1::ISocketObservable *obs, int no_threads);
    virtual ~ThreadPoolSocketObserver();
    void socketNotify(int event);
    void put(IThreadPoolMsg *m);
    IThreadPoolMsg *get();
    void run(void *p);
    int m_fd[2];
private:
    yazpp_1::ISocketObservable *m_SocketObservable;
    int m_no_threads;
    pthread_t *m_thread_id;

    std::deque<IThreadPoolMsg *> m_input;
    std::deque<IThreadPoolMsg *> m_output;
    pthread_mutex_t m_mutex_input_data;
    pthread_cond_t m_cond_input_data;
    pthread_mutex_t m_mutex_output_data;
    bool m_stop_flag;
};

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

