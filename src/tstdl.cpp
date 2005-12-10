/* $Id: tstdl.cpp,v 1.1 2005-12-10 09:59:10 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
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
	cerr << "dlopen failed for file " << argv[1] << 
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
