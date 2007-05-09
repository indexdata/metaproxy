/* $Id: tstdl.cpp,v 1.6 2007-05-09 21:23:09 adam Exp $
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

#include "config.hpp"

#include <stdlib.h>
#include <iostream>
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

using namespace std;

int main(int argc, char **argv)
{
#if HAVE_DLFCN_H
    if (argc != 3)
    {
	cerr << "bad args" << endl << "Usage: tstdl filename symbol" << endl;
	exit(1);
    }
    void *mod = dlopen(argv[1][0] ? argv[1] : 0, RTLD_NOW|RTLD_LOCAL);
    if (!mod)
    {
	cerr << "dlopen failed for file '" << argv[1] << "'\n" <<
	    "dlerror=" << dlerror() << endl;
	exit(1);
    }
    void *sym = dlsym(mod, argv[2]);
    printf("sym=%p\n", sym);
    dlclose(mod);
    exit(0);
#else
    cerr << "dl lib not enabled or not supported" << endl;
    exit(1);
#endif

}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
