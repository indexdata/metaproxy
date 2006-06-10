/* $Id: filter_auth_simple.hpp,v 1.7 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef FILTER_AUTH_SIMPLE_HPP
#define FILTER_AUTH_SIMPLE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class AuthSimple : public Base {
            class Rep;
            boost::scoped_ptr<Rep> m_p;
        public:
            AuthSimple();
            ~AuthSimple();
            void configure(const xmlNode * ptr);
            void process(metaproxy_1::Package & package) const;
        private:
            void config_userRegister(std::string filename);
            void config_targetRegister(std::string filename);
            void process_init(metaproxy_1::Package & package) const;
            void process_search(metaproxy_1::Package & package) const;
            void process_scan(metaproxy_1::Package & package) const;
            void check_targets(metaproxy_1::Package & package) const;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_auth_simple;
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
