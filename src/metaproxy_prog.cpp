/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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
#include <metaproxy/util.hpp>
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

#if HAVE_UNISTD_H
static pid_t process_group = 0;

static void sig_usr1_handler(int s)
{
    yaz_log(YLOG_LOG, "metaproxy received SIGUSR1");
    routerp->stop();
    yaz_daemon_stop();
}

static void sig_term_handler(int s)
{
    yaz_log(YLOG_LOG, "metaproxy received SIGTERM");
    yaz_log(YLOG_LOG, "metaproxy stop");
    kill(-process_group, SIGTERM); /* kill all children processes as well */
    _exit(0);
}
#endif

static void work_common(void *data)
{
#if HAVE_UNISTD_H
    process_group = getpgid(0); // save process group ID

    signal(SIGTERM, sig_term_handler);
    signal(SIGUSR1, sig_usr1_handler);
#endif
    routerp = (mp::RouterFleXML*) data;
    routerp->start();

    mp::Package pack;
    pack.router(*routerp).move();
    yaz_log(YLOG_LOG, "metaproxy stop"); /* only for graceful stop */
    _exit(0);
}

static void work_debug(void *data)
{
    work_common(data);
}
    
static void work_normal(void *data)
{
#if HAVE_UNISTD_H
    /* make the current working process group leader */
    setpgid(0, 0);
#endif
    work_common(data);
}

static int sc_main(
    yaz_sc_t s,
    int argc, char **argv)
{
    bool test_config = false;
    const char *fname = 0;
    int ret;
    char *arg;
    unsigned mode = 0;
    const char *pidfile = 0;
    const char *uid = 0;
    
    while ((ret = options("c{config}:Dh{help}l:p:tu:V{version}w:X", 
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
                " -t            test configuration\n"
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
        case 't':
            test_config = true;
            break;
        case 'u':
            uid = arg;
            break;
        case 'V':
            std::cout << VERSION;
#ifdef VERSION_SHA1
            std::cout << " " VERSION_SHA1;
#endif
            std::cout << "\n";
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
    
    yaz_log(YLOG_LOG, "metaproxy start " VERSION
#ifdef VERSION_SHA1
            " " VERSION_SHA1
#endif
        );
    
    yaz_log_xml_errors(0, YLOG_LOG);
    xmlDocPtr doc = xmlReadFile(fname,
                                NULL, 
                                XML_PARSE_XINCLUDE + XML_PARSE_NOBLANKS
                                + XML_PARSE_NSCLEAN + XML_PARSE_NONET );
    
    if (!doc)
    {
        yaz_log(YLOG_FATAL,"XML parsing failed");
        return 1;
    }
    // and perform Xinclude then
    int r = xmlXIncludeProcess(doc);
    if (r == -1)
    {
        yaz_log(YLOG_FATAL, "XInclude processing failed");
        return 1;
    }
    mp::wrbuf base_path;
    const char *last_p = strrchr(fname,
#ifdef WIN32
                                 '\\'
#else
                                 '/'
#endif
        );
    if (last_p)
        wrbuf_write(base_path, fname, last_p - fname);
    else
        wrbuf_puts(base_path, ".");
    ret = 0;
    try {
        mp::FactoryStatic factory;
        mp::RouterFleXML *router =
            new mp::RouterFleXML(doc, factory, test_config, wrbuf_cstr(base_path));
        if (!test_config)
        {
            
            yaz_sc_running(s);
            
            yaz_daemon("metaproxy", mode, mode == YAZ_DAEMON_DEBUG ?
                       work_debug : work_normal, router, pidfile, uid);
        }
    }
    catch (std::logic_error &e) {
        yaz_log(YLOG_FATAL,"std::logic error: %s" , e.what() );
        ret = 1;
    }
    catch (std::runtime_error &e) {
        yaz_log(YLOG_FATAL, "std::runtime error: %s" , e.what() );
        ret = 1;
    }
    catch ( ... ) {
        yaz_log(YLOG_FATAL, "Unknown Exception");
        ret = 1;
    }
    xmlFreeDoc(doc);
    return ret;
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

