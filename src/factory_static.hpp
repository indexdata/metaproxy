/* $Id: factory_static.hpp,v 1.5 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

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
