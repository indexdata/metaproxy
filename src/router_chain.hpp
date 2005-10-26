/* $Id: router_chain.hpp,v 1.2 2005-10-26 10:55:26 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef ROUTER_CHAIN_HPP
#define ROUTER_CHAIN_HPP

#include <stdexcept>
#include <list>

#include "router.hpp"


namespace yp2 {
    //namespace filter {
    //    class Base;
    //}
    //class Package;
    
    
    class RouterChain : public Router {
    public:
        RouterChain(){};
        virtual ~RouterChain(){};
        virtual const filter::Base *move(const filter::Base *filter,
                                   const Package *package) const;

        RouterChain & append(const filter::Base &filter);

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
