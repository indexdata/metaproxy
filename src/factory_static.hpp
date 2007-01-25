/* $Id: factory_static.hpp,v 1.6 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

#ifndef FACTORY_STATIC_HPP
#define FACTORY_STATIC_HPP

#include "factory_filter.hpp"

namespace metaproxy_1 {
    class FactoryStatic : public FactoryFilter {
    public:
        FactoryStatic();
    };
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
