/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <yaz/log.h>
#include <yaz/diagbib1.h>

#include <metaproxy/router_chain.hpp>
#include <yaz/test.h>

namespace mp = metaproxy_1;

void tst(void)
{
    try {
        mp::RouterChain router;
    }
    catch ( ... ) {
        YAZ_CHECK(0);
    }

}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    tst();
    YAZ_CHECK_TERM;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

