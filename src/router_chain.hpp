/* This file is part of Metaproxy.
   Copyright (C) 2005-2010 Index Data

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

#ifndef ROUTER_CHAIN_HPP
#define ROUTER_CHAIN_HPP


#include <metaproxy/router.hpp>

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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

