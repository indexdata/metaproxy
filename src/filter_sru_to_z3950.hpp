/* $Id: filter_sru_to_z3950.hpp,v 1.4 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

// Filter that does nothing. Use as sru_to_z3950 for new filters 
#ifndef FILTER_SRU_TO_Z3950_HPP
#define FILTER_SRU_TO_Z3950_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class SRUtoZ3950 : public Base {
            class Impl;
            boost::scoped_ptr<Impl> m_p;
        public:
            SRUtoZ3950();
            ~SRUtoZ3950();
            void configure(const xmlNode *xmlnode);
            void process(metaproxy_1::Package & package) const;
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_sru_to_z3950;
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
