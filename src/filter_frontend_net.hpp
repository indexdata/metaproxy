/* $Id: filter_frontend_net.hpp,v 1.6 2005-10-29 22:23:36 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_FRONTEND_NET_HPP
#define FILTER_FRONEND_NET_HPP

#include <stdexcept>
#include <vector>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class FrontendNet : public Base {
        public:
            FrontendNet::FrontendNet();
            void process(yp2::Package & package) const;
            const std::string type() const {
                return "FrontendNet";
            };
        private:
            int m_no_threads;
            std::vector<std::string> m_ports;
            int m_listen_duration;
        public:
            /// set function - left val in assignment
            std::vector<std::string> &ports();
            int &listen_duration();
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
