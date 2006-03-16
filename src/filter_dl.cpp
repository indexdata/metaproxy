/* $Id: filter_dl.cpp,v 1.5 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class Filter_dl: public mp::filter::Base {
        public:
            void process(mp::Package & package) const;
        };
    }
}

void mp::filter::Filter_dl::process(mp::Package & package) const
{
    package.data() = 42;   // magic checked in test_filter_factory
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::Filter_dl;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_dl = {
        0,
        "dl",
        filter_creator
    };
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
