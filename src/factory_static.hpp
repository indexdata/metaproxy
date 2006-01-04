/* $Id: factory_static.hpp,v 1.3 2006-01-04 14:30:51 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FACTORY_STATIC_HPP
#define FACTORY_STATIC_HPP

#include "factory_filter.hpp"

namespace yp2 {
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
