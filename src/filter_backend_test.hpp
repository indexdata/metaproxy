/* $Id: filter_backend_test.hpp,v 1.10 2006-09-28 11:56:54 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef FILTER_BACKEND_TEST_HPP
#define FILTER_BACKEND_TEST_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class BackendTest : public Base {
            class Rep;
        public:
            ~BackendTest();
            BackendTest();
            void process(metaproxy_1::Package & package) const;
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_backend_test;
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
