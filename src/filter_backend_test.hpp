/* $Id: filter_backend_test.hpp,v 1.1 2005-10-25 11:48:30 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_BACKEND_TEST_HPP
#define FILTER_BACKEND_TEST_HPP

#include <stdexcept>
#include <list>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Backend_test : public Base {
            class Rep;
        public:
            ~Backend_test();
            Backend_test();
            void process(yp2::Package & package) const;
        private:
            Rep *m_p;
        };
    }
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
