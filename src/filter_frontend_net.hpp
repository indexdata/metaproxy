/* $Id: filter_frontend_net.hpp,v 1.11 2006-01-09 13:43:59 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_FRONTEND_NET_HPP
#define FILTER_FRONTEND_NET_HPP

#include <stdexcept>
#include <vector>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class FrontendNet : public Base {
            class ZAssocServerChild;
        public:
            FrontendNet::FrontendNet();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
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

extern "C" {
    extern struct yp2_filter_struct yp2_filter_frontend_net;
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
