/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

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

#include <signal.h>
#include <errno.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <fcntl.h>
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
#include <string.h>

#include <deque>

#include <yazpp/socket-observer.h>
#include <yaz/log.h>

#include "pipe.hpp"

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    class Pipe::Rep : public boost::noncopyable {
        friend class Pipe;
        Rep();
        int m_fd[2];
        int m_socket;
        bool nonblock(int s);
        void close(int &fd);
    };
}

using namespace mp;

void Pipe::Rep::close(int &fd)
{
#ifdef WIN32
    if (fd != -1)
        ::closesocket(fd);
#else
    if (fd != -1)
        ::close(fd);
#endif
    fd = -1;
}

Pipe::Rep::Rep()
{
    m_fd[0] = m_fd[1] = -1;
    m_socket = -1;
}

bool Pipe::Rep::nonblock(int s)
{
#ifdef WIN32
    unsigned long tru = 1;
    if (ioctlsocket(s, FIONBIO, &tru) < 0)
        return false;
#else
    if (fcntl(s, F_SETFL, O_NONBLOCK) < 0)
        return false;
#ifndef MSG_NOSIGNAL
    signal (SIGPIPE, SIG_IGN);
#endif
#endif
    return true;
}

Pipe::Pipe(int port_to_use) : m_p(new Rep)
{
#ifdef WIN32
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    if (WSAStartup( wVersionRequested, &wsaData ))
        throw Pipe::Error("WSAStartup failed");
#else
    port_to_use = 0;  // we'll just use pipe on Unix
#endif
    if (port_to_use)
    {
        // create server socket
        m_p->m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_p->m_socket < 0)
            throw Pipe::Error("could not create socket");
#ifndef WIN32
        unsigned long one = 1;
        if (setsockopt(m_p->m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)
                       &one, sizeof(one)) < 0)
            throw Pipe::Error("setsockopt error");
#endif
        // bind server socket
        struct sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_port = htons(port_to_use);
        add.sin_addr.s_addr = INADDR_ANY;
        struct sockaddr *addr = ( struct sockaddr *) &add;

        if (bind(m_p->m_socket, addr, sizeof(struct sockaddr_in)))
            throw Pipe::Error("could not bind on socket");

        if (listen(m_p->m_socket, 3) < 0)
            throw Pipe::Error("could not listen on socket");

        // client socket
        unsigned int tmpadd;
        tmpadd = (unsigned) inet_addr("127.0.0.1");
        if (tmpadd)
            memcpy(&add.sin_addr.s_addr, &tmpadd, sizeof(struct in_addr));
        else
            throw Pipe::Error("inet_addr failed");

        m_p->m_fd[1] = socket(AF_INET, SOCK_STREAM, 0);
        if (m_p->m_fd[1] < 0)
            throw Pipe::Error("could not create socket");

        m_p->nonblock(m_p->m_fd[1]);

        if (connect(m_p->m_fd[1], addr, sizeof(*addr)) < 0)
        {
#ifdef WIN32
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                throw Pipe::Error("could not connect to socket");
#else
            if (errno != EINPROGRESS)
                throw Pipe::Error("could not connect to socket");
#endif
        }

        // server accept
        struct sockaddr caddr;
#ifdef WIN32
        int caddr_len = sizeof(caddr);
#else
        socklen_t caddr_len = sizeof(caddr);
#endif
        m_p->m_fd[0] = accept(m_p->m_socket, &caddr, &caddr_len);
        if (m_p->m_fd[0] < 0)
            throw Pipe::Error("could not accept on socket");

        // complete connect
        fd_set write_set;
        FD_ZERO(&write_set);
        FD_SET(m_p->m_fd[1], &write_set);
        int r = select(m_p->m_fd[1]+1, 0, &write_set, 0, 0);
        if (r != 1)
            throw Pipe::Error("could not complete connect");

        m_p->close(m_p->m_socket);
    }
    else
    {
#ifndef WIN32
        if (pipe(m_p->m_fd))
            throw Pipe::Error("pipe failed");
        else
        {
            assert(m_p->m_fd[0] >= 0);
            assert(m_p->m_fd[1] >= 0);
        }
#endif
    }
}

Pipe::~Pipe()
{
    m_p->close(m_p->m_fd[0]);
    m_p->close(m_p->m_fd[1]);
    m_p->close(m_p->m_socket);
#ifdef WIN32
    WSACleanup();
#endif
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

