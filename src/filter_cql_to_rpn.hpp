/* $Id: filter_cql_to_rpn.hpp,v 1.1 2007-01-12 10:16:21 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef FILTER_CQL_TO_RPN_HPP
#define FILTER_CQL_TO_RPN_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class CQL_to_RPN : public Base {
            class Rep;
        public:
            CQL_to_RPN();
            ~CQL_to_RPN();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        private:
            class Impl;
            boost::scoped_ptr<Rep> m_p;
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
