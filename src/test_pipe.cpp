/* $Id: test_pipe.cpp,v 1.1 2005-11-07 12:32:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include <yaz++/socket-manager.h>

#include <iostream>
#include <stdexcept>

#include "util.hpp"
#include "pipe.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

class My_Timer_Thread : public yazpp_1::ISocketObserver {
private:
    yazpp_1::ISocketObservable *m_obs;
    yp2::Pipe m_pipe;
    bool m_timeout;
public:
    My_Timer_Thread(yazpp_1::ISocketObservable *obs, int duration);
    void socketNotify(int event);
    bool timeout() { return m_timeout; };
};


My_Timer_Thread::My_Timer_Thread(yazpp_1::ISocketObservable *obs,
				 int duration) : 
    m_obs(obs), m_pipe(0), m_timeout(false)
{
    obs->addObserver(m_pipe.read_fd(), this);
    obs->maskObserver(this, yazpp_1::SOCKET_OBSERVE_READ);
    obs->timeoutObserver(this, duration);
}

void My_Timer_Thread::socketNotify(int event)
{
    m_timeout = true;
    m_obs->deleteObserver(this);
}

BOOST_AUTO_TEST_CASE( test_pipe_1 )
{
    yazpp_1::SocketManager mySocketManager;
    
    yp2::Pipe pipe(0);

    My_Timer_Thread t(&mySocketManager, 0);

    while (mySocketManager.processEvent() > 0)
        if (t.timeout())
            break;
    BOOST_CHECK (t.timeout());
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
