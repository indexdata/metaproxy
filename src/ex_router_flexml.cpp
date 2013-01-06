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

#include "config.hpp"

#include <yaz/options.h>

#include <iostream>
#include <stdexcept>

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include "router_flexml.hpp"
#include "factory_static.hpp"

namespace mp = metaproxy_1;

int main(int argc, char **argv)
{
    try
    {
        int ret;
        char *arg;
        char *fname = 0;

        while ((ret = options("h{help}c{config}:",
                              argv, argc, &arg)) != -2)
        {
            switch(ret)
            {
            case -1:
                std::cerr << "bad option " << arg << std::endl;
            case 'h':
                std::cerr << "ex_router_flexml\n"
                    " -h|--help         help\n"
                    " -c|--config fname configuation\n"
                          << std::endl;
                std::exit(1);
            case 'c':
                fname = arg;
            }
        }

        xmlDocPtr doc = 0;
        if (fname)
        {
            doc = xmlParseFile(fname);
            if (!doc)
            {
                std::cerr << "xmlParseFile failed\n";
                std::exit(1);
            }
        }
        else
        {
            std::cerr << "No configuration given\n";
            std::exit(1);
        }
        if (doc)
        {
            mp::FactoryStatic factory;
            mp::RouterFleXML router(doc, factory, false, 0);

	    mp::Package pack;

            pack.router(router).move();

            xmlFreeDoc(doc);
        }
    }
    catch ( ... ) {
        std::cerr << "Unknown Exception" << std::endl;
        throw;
        std::exit(1);
    }
    std::exit(0);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

