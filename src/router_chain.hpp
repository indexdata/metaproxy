/* $Id: router_chain.hpp,v 1.1 2005-10-26 10:21:03 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef ROUTER_CHAIN_HPP
#define ROUTER_CHAIN_HPP

#include <stdexcept>
#include <list>

#include "router.hpp"


namespace yp2 {
    namespace filter {
        class Base;
    }
    class Package;
    
    
    class RouterChain : public Router {
    public:
        RouterChain(){};
        virtual ~RouterChain(){};
        virtual const filter::Base *move(const filter::Base *filter,
                                   const Package *package) const {
            std::list<const filter::Base *>::const_iterator it;
            it = m_filter_list.begin();
            if (filter)
                {
                    for (; it != m_filter_list.end(); it++)
                        if (*it == filter)
                            {
                                it++;
                                break;
                            }
                }
            if (it == m_filter_list.end())
                {
                    //throw RouterException("no routing rules known");
                    return 0;
                }
            return *it;
        };
        virtual void configure(){};
        RouterChain & rule(const filter::Base &filter){
            m_filter_list.push_back(&filter);
            return *this;
        }
    protected:
        std::list<const filter::Base *> m_filter_list;
    private:
        /// disabled because class is singleton
        RouterChain(const RouterChain &);

        /// disabled because class is singleton
        RouterChain& operator=(const RouterChain &);
    };
  

  
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
