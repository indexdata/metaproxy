/* $Id: filter_session_shared.hpp,v 1.1 2005-11-14 23:35:22 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_SESSION_SHARED_HPP
#define FILTER_SESSION_SHARED_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Session_shared : public Base {
            class Rep;
            class InitKey;
            class List;
        public:
            ~Session_shared();
            Session_shared();
            void process(yp2::Package & package) const;
        private:
            boost::scoped_ptr<Rep> m_p;
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
