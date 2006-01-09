/* $Id: router_chain.cpp,v 1.3 2006-01-09 13:43:59 adam Exp $
   Copyright (c) 2005, Index Data.
   
   %LICENSE%
*/

#include "router_chain.hpp"

#include <list>

namespace yp2 
{
    class ChainPos;

    class RouterChain::Rep {
        friend class RouterChain;
        friend class RouterChain::Pos;
        std::list<const filter::Base *> m_filter_list;
    };

#if ROUTE_POS
    class RouterChain::Pos : public RoutePos {
    public:
        virtual const filter::Base *move();
        virtual RoutePos *clone();
        virtual ~Pos();
        std::list<const filter::Base *>::const_iterator it;
        yp2::RouterChain::Rep *m_p;
    };
#endif
};

yp2::RouterChain::RouterChain() : m_p(new yp2::RouterChain::Rep)
{
}

yp2::RouterChain::~RouterChain()
{
}

#if ROUTE_POS
const yp2::filter::Base *yp2::RouterChain::Pos::move()
{
    if (it == m_p->m_filter_list.end())
        return 0;
    const yp2::filter::Base *f = *it;
    it++;
    return f;
}

yp2::RoutePos *yp2::RouterChain::createpos() const
{
    yp2::RouterChain::Pos *p = new yp2::RouterChain::Pos;
    p->it = m_p->m_filter_list.begin();
    p->m_p = m_p.get();
    return p;
}

yp2::RoutePos *yp2::RouterChain::Pos::clone()
{
    yp2::RouterChain::Pos *p = new yp2::RouterChain::Pos;
    p->it = it;
    p->m_p = m_p;
    return p;
}


yp2::RouterChain::Pos::~Pos()
{
}
#else
const yp2::filter::Base *yp2::RouterChain::move(const filter::Base *filter,
                                                const Package *package) const {
    std::list<const filter::Base *>::const_iterator it;
    it = m_p->m_filter_list.begin();
    if (filter)
    {
        for (; it != m_p->m_filter_list.end(); it++)
            if (*it == filter)
            {
                it++;
                break;
            }
    }
    if (it == m_p->m_filter_list.end())
    {
        //throw RouterException("no routing rules known");
        return 0;
    }
    return *it;
}
#endif

yp2::RouterChain & yp2::RouterChain::append(const filter::Base &filter)
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
