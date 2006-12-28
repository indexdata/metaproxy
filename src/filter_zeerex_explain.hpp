/* $Id: filter_zeerex_explain.hpp,v 1.1 2006-12-28 14:59:44 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

// Filter that constructs valid ZeeRex Explain SRU responses 
#ifndef FILTER_ZEEREX_EXPLAIN_HPP
#define FILTER_ZEEREX_EXPLAIN_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class ZeeRexExplain : public Base {
            class Impl;
            boost::scoped_ptr<Impl> m_p;
        public:
            ZeeRexExplain();
            ~ZeeRexExplain();
            void configure(const xmlNode *xmlnode);
            void process(metaproxy_1::Package & package) const;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_zeerex_explain;
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
