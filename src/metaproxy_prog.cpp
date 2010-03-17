/* This file is part of Metaproxy.
   Copyright (C) 2005-2010 Index Data

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

#include <yaz/sc.h>
#include <iostream>
#include <stdexcept>
#include <libxml/xinclude.h>

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include "router_flexml.hpp"
#include "factory_static.hpp"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#ifdef WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#endif

namespace mp = metaproxy_1;

mp::RouterFleXML *routerp = 0;

static void sig_term_handler(int s)
{
    if (routerp)
    {
        delete routerp;
    }
    exit(0);
}

static void handler(void *data)
{
    routerp = (mp::RouterFleXML*) data;
    
    signal(SIGTERM, sig_term_handler);
    mp::Package pack;
    pack.router(*routerp).move();
}
    
static int sc_main(
    yaz_sc_t s,
    int argc, char **argv)
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
#ifdef WIN32
                    " -install      install windows service\n"
                    " -remove       remove windows service\n"
#endif

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
                return 0;
                break;
            case 'w':
                if (
#ifdef WIN32
                    _chdir(arg)
#else
                    chdir(arg)
#endif
                   ) 
                {
                    std::cerr << "chdir " << arg << " failed" << std::endl;
                    return 1;
                }
            case 'X':
                mode = YAZ_DAEMON_DEBUG;
                break;
            case -1:
                std::cerr << "bad option: " << arg << std::endl;
                return 1;
            }
        }
        if (!fname)
        {
            std::cerr << "No configuration given; use -h for help\n";
            return 1;
        }

        yaz_log(YLOG_LOG, "Metaproxy " VERSION " started");
        xmlDocPtr doc = xmlReadFile(fname,
                                    NULL, 
                                    XML_PARSE_XINCLUDE + XML_PARSE_NOBLANKS
                                    + XML_PARSE_NSCLEAN + XML_PARSE_NONET );
        
        if (!doc)
        {
            yaz_log (YLOG_FATAL,"XML parsing failed");
            return 1;
        }
        // and perform Xinclude then
        int r = xmlXIncludeProcess(doc);
        if (r == -1)
        {
            yaz_log(YLOG_FATAL, "XInclude processing failed");
            return 1;
        }
        WRBUF base_path = wrbuf_alloc();
        const char *last_p = strrchr(fname,
#ifdef WIN32
                                     '\\'
#else
                                     '/'
#endif
            );
        if (last_p)
            wrbuf_write(base_path, fname, last_p - fname);
        
        mp::FactoryStatic factory;
        mp::RouterFleXML *router =
            new mp::RouterFleXML(doc, factory, false, wrbuf_cstr(base_path));
        wrbuf_destroy(base_path);

        yaz_sc_running(s);

        yaz_daemon("metaproxy", mode, handler, router, pidfile, uid);
    }
    catch (std::logic_error &e) {
        yaz_log (YLOG_FATAL,"std::logic error: %s" , e.what() );
        return 1;
    }
    catch (std::runtime_error &e) {
        yaz_log(YLOG_FATAL, "std::runtime error: %s" , e.what() );
        return 1;
    }
    catch ( ... ) {
        yaz_log(YLOG_FATAL, "Unknown Exception");
        return 1;
    }
    return 0;
}

static void sc_stop(yaz_sc_t s)
{
    return;
}

int main(int argc, char **argv)
{
    int ret;
    yaz_sc_t s = yaz_sc_create("metaproxy", "metaproxy");

    ret = yaz_sc_program(s, argc, argv, sc_main, sc_stop);

    yaz_sc_destroy(&s);
    exit(ret);
}



/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

