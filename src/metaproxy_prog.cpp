/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

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

#include <yaz/log.h>
#include <yaz/options.h>
#include <yaz/daemon.h>

#include <iostream>
#include <stdexcept>
#include <libxml/xinclude.h>

#include "filter.hpp"
#include "package.hpp"
#include "router_flexml.hpp"
#include "factory_static.hpp"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace mp = metaproxy_1;

static void handler(void *data)
{
    mp::RouterFleXML *routerp = (mp::RouterFleXML*) data;

    mp::Package pack;
    pack.router(*routerp).move();
}
    
int main(int argc, char **argv)
{
    try 
    {
        const char *fname = 0;
        int ret;
        char *arg;
        unsigned mode = 0;
        const char *pidfile = 0;
        const char *uid = 0;

        while ((ret = options("c{config}:Dh{help}l:p:u:V{version}w:X", 
                              argv, argc, &arg)) != -2)
        {
            switch (ret)
            {
            case 'c':
                fname = arg;
                break;
            case 'D':
                mode = YAZ_DAEMON_FORK|YAZ_DAEMON_KEEPALIVE;
                break;
            case 'h':
                std::cerr << "metaproxy\n"
                    " -h|--help     help\n"
                    " -V|--version  version\n"
                    " -c|--config f config filename\n"
                    " -D            daemon and keepalive operation\n"
                    " -l f          log file f\n"
                    " -p f          pid file f\n"
                    " -u id         change uid to id\n"
                    " -w dir        changes working directory to dir\n"
                    " -X            debug mode (no fork/daemon mode)\n"
                          << std::endl;
                break;
            case 'l':
                yaz_log_init_file(arg);
                break;
            case 'p':
                pidfile = arg;
                break;
            case 'u':
                uid = arg;
                break;
            case 'V':
                std::cout << VERSION "\n";
                std::exit(0);
                break;
            case 'w':
                if (chdir(arg)) 
                {
                    std::cerr << "chdir " << arg << " failed" << std::endl;
                    std::exit(1);
                }
            case 'X':
                mode = YAZ_DAEMON_DEBUG;
                break;
            case -1:
                std::cerr << "bad option: " << arg << std::endl;
                std::exit(1);
            }
        }
        if (!fname)
        {
            std::cerr << "No configuration given; use -h for help\n";
            std::exit(1);
        }

        yaz_log(YLOG_LOG, "Metaproxy " VERSION " started");
        xmlDocPtr doc = xmlReadFile(fname,
                                    NULL, 
                                    XML_PARSE_XINCLUDE + XML_PARSE_NOBLANKS
                                    + XML_PARSE_NSCLEAN + XML_PARSE_NONET );
        
        if (!doc)
        {
            std::cerr << "XML parsing failed\n";
            std::exit(1);
        }
        // and perform Xinclude then
        if (xmlXIncludeProcess(doc) > 0) {
            std::cerr << "processing XInclude directive\n";
        }
        mp::FactoryStatic factory;
        mp::RouterFleXML router(doc, factory, false);

        yaz_daemon("metaproxy", mode, handler, &router, pidfile, uid);
    }
    catch (std::logic_error &e) {
        std::cerr << "std::logic error: " << e.what() << "\n";
        std::exit(1);
    }
    catch (std::runtime_error &e) {
        std::cerr << "std::runtime error: " << e.what() << "\n";
        std::exit(1);
    }
    catch ( ... ) {
        std::cerr << "Unknown Exception" << std::endl;
        std::exit(1);
    }
    std::exit(0);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
