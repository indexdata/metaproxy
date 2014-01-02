/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

#include <metaproxy/router_chain.hpp>
#include <metaproxy/filter.hpp>

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

void mp::RouterChain::start()
{
    std::list<const filter::Base *>::const_iterator it;

    for (it = m_p->m_filter_list.begin(); it != m_p->m_filter_list.end(); it++)
        (*it)->start();
}

void mp::RouterChain::stop(int signo)
{
    std::list<const filter::Base *>::const_iterator it;

    for (it = m_p->m_filter_list.begin(); it != m_p->m_filter_list.end(); it++)
        (*it)->stop(signo);
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

