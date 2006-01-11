/* $Id: filter.cpp,v 1.6 2006-01-11 14:58:28 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"

void yp2::filter::Base::configure(const xmlNode * ptr)
{
    yp2::xml::check_empty(ptr);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
