/* $Id: test_pipe.cpp,v 1.6 2006-03-29 13:44:45 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include <yazpp/socket-manager.h>

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
    bool m_timeout;
public:
    Timer(yazpp_1::ISocketObservable *obs, int duration);
    void socketNotify(int event);
    bool timeout() { return m_timeout; };
};


Timer::Timer(yazpp_1::ISocketObservable *obs,
				 int duration) : 
    m_obs(obs), m_pipe(9122), m_timeout(false)
{
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
}

void Timer::socketNotify(int event)
{
    m_timeout = true;
    m_obs->deleteObserver(this);
}

BOOST_AUTO_UNIT_TEST( test_pipe_1 )
{
    yazpp_1::SocketManager mySocketManager;
    
    Timer t(&mySocketManager, 0);

    while (mySocketManager.processEvent() > 0)
        if (t.timeout())
            break;
    BOOST_CHECK(t.timeout());
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
