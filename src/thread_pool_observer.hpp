/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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

#ifndef YP2_THREAD_POOL_OBSERVER_HPP
#define YP2_THREAD_POOL_OBSERVER_HPP

#include <boost/scoped_ptr.hpp>

#include <yazpp/socket-observer.h>
#include <yaz/yconfig.h>

namespace metaproxy_1 {
    class IThreadPoolMsg {
    public:
        virtual IThreadPoolMsg *handle() = 0;
        virtual void result(const char *info) = 0;
        virtual ~IThreadPoolMsg();
        virtual bool cleanup(void *info) = 0;
    };

    class ThreadPoolSocketObserver : public yazpp_1::ISocketObserver {
        class Rep;
        class Worker;
    public:
        ThreadPoolSocketObserver(yazpp_1::ISocketObservable *obs,
                                 int no_threads);
        virtual ~ThreadPoolSocketObserver();
        void put(IThreadPoolMsg *m);
        void cleanup(IThreadPoolMsg *m, void *info);
        IThreadPoolMsg *get();
        void run(void *p);
        void get_thread_info(int &tbusy, int &total);
    private:
        void socketNotify(int event);
        boost::scoped_ptr<Rep> m_p;

    };
}
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

