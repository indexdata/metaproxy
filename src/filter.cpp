/* $Id: filter.cpp,v 1.8 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"

namespace mp = metaproxy_1;

void mp::filter::Base::configure(const xmlNode * ptr)
{
    mp::xml::check_empty(ptr);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
