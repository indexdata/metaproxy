
/* $Id: pipe.cpp,v 1.1 2005-11-07 12:32:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */
#include "config.hpp"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <stdio.h>

#include <deque>

#include <yaz++/socket-observer.h>
#include <yaz/log.h>

#include "pipe.hpp"

namespace yp2 {
    class Pipe::Rep : public boost::noncopyable {
        friend class Pipe;
        Rep();
        int m_fd[2];
        int m_socket;
    };
}

using namespace yp2;

Pipe::Rep::Rep()
{
    m_fd[0] = m_fd[1] = -1;
    m_socket = -1;
}

Pipe::Pipe(int port_to_use) : m_p(new Rep)
{
    if (port_to_use)
    {
        m_p->m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_p->m_socket < 0)
            throw Pipe::Error("could not create socket");

        m_p->m_fd[1] = socket(AF_INET, SOCK_STREAM, 0);
        if (m_p->m_fd[1] < 0)
            throw Pipe::Error("could not create socket");
        
        struct sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_port = htons(port_to_use);
        add.sin_addr.s_addr = INADDR_ANY;
        struct sockaddr *addr = ( struct sockaddr *) &add;
        
        if (bind(m_p->m_socket, addr, sizeof(struct sockaddr_in)))
            throw Pipe::Error("could not bind on socket");
        
        if (listen(m_p->m_socket, 3) < 0)
            throw Pipe::Error("could not listen on socket");

        struct sockaddr caddr;
        socklen_t caddr_len = sizeof(caddr);
        m_p->m_fd[0] = accept(m_p->m_socket, &caddr, &caddr_len);
        if (m_p->m_fd[0] < 0)
            throw Pipe::Error("could not accept on socket");
        
        if (connect(m_p->m_fd[1], addr, sizeof(addr)) < 0)
            throw Pipe::Error("could not connect to socket");
    }
    else
    {
        m_p->m_socket = 0;
        pipe(m_p->m_fd);
    }
}

Pipe::~Pipe()
{
    if (m_p->m_fd[0] != -1)
        close(m_p->m_fd[0]);
    if (m_p->m_fd[1] != -1)
        close(m_p->m_fd[1]);
    if (m_p->m_socket != -1)
        close(m_p->m_socket);
}

int &Pipe::read_fd() const
{
    return m_p->m_fd[0];
}

int &Pipe::write_fd() const
{
    return m_p->m_fd[1];
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

