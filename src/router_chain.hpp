/* $Id: router_chain.hpp,v 1.8 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

#ifndef ROUTER_CHAIN_HPP
#define ROUTER_CHAIN_HPP


#include "router.hpp"

#include <boost/scoped_ptr.hpp>
#include <stdexcept>

namespace metaproxy_1 {
    class RouterChain : public Router {
        class Rep;
        class Pos;
    public:
        RouterChain();
        virtual ~RouterChain();
        virtual RoutePos *createpos() const;
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
