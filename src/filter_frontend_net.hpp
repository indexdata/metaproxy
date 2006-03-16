/* $Id: filter_frontend_net.hpp,v 1.13 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#ifndef FILTER_FRONTEND_NET_HPP
#define FILTER_FRONTEND_NET_HPP

#include <boost/scoped_ptr.hpp>

#include <stdexcept>
#include <vector>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class FrontendNet : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            FrontendNet();
            ~FrontendNet();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        public:
            /// set ports
            std::vector<std::string> &ports();
            // set liten duraction (number of seconcds to listen)
            int &listen_duration();
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_frontend_net;
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
