/* $Id: filter_cql_to_rpn.hpp,v 1.5 2008-02-20 15:07:51 adam Exp $
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

#ifndef FILTER_CQL_TO_RPN_HPP
#define FILTER_CQL_TO_RPN_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class CQLtoRPN : public Base {
            class Impl;
            boost::scoped_ptr<Impl> m_p;
        public:
            CQLtoRPN();
            ~CQLtoRPN();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only);
        private:
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_cql_to_rpn;
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
