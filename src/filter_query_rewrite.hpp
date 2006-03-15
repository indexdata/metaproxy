/* $Id: filter_query_rewrite.hpp,v 1.3 2006-03-15 14:55:17 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

// Filter that rewrites RPN queries using Regular Expressions
#ifndef FILTER_QUERY_REWRITE_HPP
#define FILTER_QUERY_REWRITE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class QueryRewrite : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            QueryRewrite();
            ~QueryRewrite();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_query_rewrite;
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
