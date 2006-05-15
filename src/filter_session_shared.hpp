/* $Id: filter_session_shared.hpp,v 1.5 2006-05-15 10:34:40 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#ifndef FILTER_SESSION_SHARED_HPP
#define FILTER_SESSION_SHARED_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class SessionShared : public Base {
            class Rep;
            class InitKey;
            class List;

            struct Frontend;
            class BackendClass;
            typedef boost::shared_ptr<Frontend> FrontendPtr;
        public:
            ~SessionShared();
            SessionShared();
            void process(metaproxy_1::Package & package) const;
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_session_shared;
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
