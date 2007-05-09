/* $Id: router_chain.cpp,v 1.9 2007-05-09 21:23:09 adam Exp $
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

#include "router_chain.hpp"

#include <list>

namespace mp = metaproxy_1;

namespace metaproxy_1
{
    class ChainPos;

    class RouterChain::Rep {
        friend class RouterChain;
        friend class RouterChain::Pos;
        std::list<const filter::Base *> m_filter_list;
    };
    class RouterChain::Pos : public RoutePos {
    public:
        virtual const filter::Base *move(const char *route);
        virtual RoutePos *clone();
        virtual ~Pos();
        std::list<const filter::Base *>::const_iterator it;
        mp::RouterChain::Rep *m_p;
    };
}

mp::RouterChain::RouterChain() : m_p(new mp::RouterChain::Rep)
{
}

mp::RouterChain::~RouterChain()
{
}

const mp::filter::Base *mp::RouterChain::Pos::move(const char *route)
{
    if (it == m_p->m_filter_list.end())
        return 0;
    const mp::filter::Base *f = *it;
    it++;
    return f;
}

mp::RoutePos *mp::RouterChain::createpos() const
{
    mp::RouterChain::Pos *p = new mp::RouterChain::Pos;
    p->it = m_p->m_filter_list.begin();
    p->m_p = m_p.get();
    return p;
}

mp::RoutePos *mp::RouterChain::Pos::clone()
{
    mp::RouterChain::Pos *p = new mp::RouterChain::Pos;
    p->it = it;
    p->m_p = m_p;
    return p;
}


mp::RouterChain::Pos::~Pos()
{
}

mp::RouterChain & mp::RouterChain::append(const filter::Base &filter)
{
    m_p->m_filter_list.push_back(&filter);
    return *this;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
