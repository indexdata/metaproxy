/* $Id: filter_record_transform.hpp,v 1.2 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

// Filter that does nothing. Use as RecordTransform for new filters 
#ifndef FILTER_RECORD_TRANSFORM_HPP
#define FILTER_RECORD_TRANSFORM_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace metaproxy_1 {
    namespace filter {
        class RecordTransform : public Base {
            class Impl;
            boost::scoped_ptr<Impl> m_p;
        public:
            RecordTransform();
            ~RecordTransform();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_record_transform;
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

