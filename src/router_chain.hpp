/* $Id: router_chain.hpp,v 1.3 2005-11-10 23:10:42 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef ROUTER_CHAIN_HPP
#define ROUTER_CHAIN_HPP


#include "router.hpp"

#include <boost/scoped_ptr.hpp>
#include <stdexcept>

namespace yp2 {
    class RouterChain : public Router {
        class Rep;
    public:
        RouterChain();
        virtual ~RouterChain();
        virtual const filter::Base *move(const filter::Base *filter,
                                         const Package *package) const;
        
        RouterChain & append(const filter::Base &filter);
    private:
        boost::scoped_ptr<Rep> m_p;
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
