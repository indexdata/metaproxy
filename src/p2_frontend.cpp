/* $Id: p2_frontend.cpp,v 1.1.1.1 2005-10-06 09:37:25 marc Exp $
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

#include <yaz/log.h>
#include <yaz/diagbib1.h>
#include "p2_frontend.h"

using namespace yazpp_1;
using namespace std;

P2_Frontend::P2_Frontend(IPDU_Observable *the_PDU_Observable,
                         Msg_Thread *my_thread, P2_Server *server)
    :  Z_Assoc(the_PDU_Observable)
{
    m_my_thread = my_thread;
    m_server = server;
    m_no_requests = 0;
    m_delete_flag = 0;
    yaz_log(YLOG_LOG, "Construct P2_Frontend=%p", this);
}


IPDU_Observer *P2_Frontend::sessionNotify(IPDU_Observable
                                          *the_PDU_Observable, int fd)
{
    return 0;
}

P2_Frontend::~P2_Frontend()
{
    yaz_log(YLOG_LOG, "Destroy P2_Frontend=%p", this);

    list<P2_FrontResultSet *>::iterator it;
    
    for (it = m_resultSets.begin(); it != m_resultSets.end(); it++)
    {
        delete *it;
        *it = 0;
    }
}

void P2_Frontend::recv_GDU(Z_GDU *z_pdu, int len)
{
    GDU *gdu = new GDU(z_pdu);

    P2_Msg *m = new P2_Msg(gdu, this, m_server);
    m_no_requests++;
    m_my_thread->put(m);  
}

void P2_Frontend::failNotify()
{
    m_delete_flag = 1;
    if (m_no_requests == 0)
        delete this;
    
}

void P2_Frontend::timeoutNotify()
{
    m_delete_flag = 1;
    if (m_no_requests == 0)
        delete this;
}

void P2_Frontend::connectNotify()
{

}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
