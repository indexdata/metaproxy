/* $Id: filter.cpp,v 1.11 2008-02-20 15:07:51 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"

namespace mp = metaproxy_1;

void mp::filter::Base::configure(const xmlNode * ptr, bool test_only)
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
