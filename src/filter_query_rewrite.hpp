/* $Id: filter_query_rewrite.hpp,v 1.5 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

// Filter that rewrites RPN queries using Regular Expressions
#ifndef FILTER_QUERY_REWRITE_HPP
#define FILTER_QUERY_REWRITE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class QueryRewrite : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            QueryRewrite();
            ~QueryRewrite();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_query_rewrite;
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
