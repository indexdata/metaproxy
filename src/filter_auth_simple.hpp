/* $Id: filter_auth_simple.hpp,v 1.1 2006-01-12 10:04:34 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_AUTH_SIMPLE_HPP
#define FILTER_AUTH_SIMPLE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class AuthSimple : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            AuthSimple();
            ~AuthSimple();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_auth_simple;
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
