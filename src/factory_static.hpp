/* $Id: factory_static.hpp,v 1.1 2006-01-04 11:19:04 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FACTORY_STATIC_HPP
#define FACTORY_STATIC_HPP

#include "filter_factory.hpp"

namespace yp2 {
    class FactoryStatic {
    public:
        FactoryStatic(yp2::FilterFactory &factory);
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
