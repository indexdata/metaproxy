/* $Id: router_chain.cpp,v 1.6 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.
   
   %LICENSE%
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
