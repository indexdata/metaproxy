/* $Id: test_pipe.cpp,v 1.11 2007-05-09 21:23:09 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include "config.hpp"
#include <errno.h>
#include <yazpp/socket-manager.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN32
#include <winsock.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <iostream>
#include <stdexcept>

#include "util.hpp"
#include "pipe.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

class Timer : public yazpp_1::ISocketObserver {
private:
    yazpp_1::ISocketObservable *m_obs;
    mp::Pipe m_pipe;
    bool m_data;
    bool m_timeout;
public:
    Timer(yazpp_1::ISocketObservable *obs, int duration);
    void socketNotify(int event);
    bool timeout() { return m_timeout; };
    bool data() { return m_data; };
};


Timer::Timer(yazpp_1::ISocketObservable *obs,
				 int duration) : 
    m_obs(obs), m_pipe(9122), m_data(false), m_timeout(false)
{
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
#ifdef WIN32
    int r = send(m_pipe.write_fd(), "", 1, 0);
#else
    int r = write(m_pipe.write_fd(), "", 1);
#endif
    if (r == -1)
    {
        std::cout << "Error write: "<< strerror(errno) << std::endl;
    }
    BOOST_CHECK_EQUAL(r, 1);
}

void Timer::socketNotify(int event)
{
    if (event & yazpp_1::SOCKET_OBSERVE_READ)
    {
        m_data = true;
        char buf[3];
#ifdef WIN32
        int r = recv(m_pipe.read_fd(), buf, 1, 0);
#else
        int r = read(m_pipe.read_fd(), buf, 1);
#endif
        if (r == -1)
        {
            std::cout << "Error read: "<< strerror(errno) << std::endl;
        }
    }
    else if (event && yazpp_1::SOCKET_OBSERVE_TIMEOUT)
    {
        m_timeout = true;
        m_obs->deleteObserver(this);
    }
}

BOOST_AUTO_UNIT_TEST( test_pipe_1 )
{
    yazpp_1::SocketManager mySocketManager;
    
    Timer t(&mySocketManager, 1);

    while (mySocketManager.processEvent() > 0)
        if (t.timeout())
            break;
    BOOST_CHECK(t.timeout());
    BOOST_CHECK(t.data());
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
