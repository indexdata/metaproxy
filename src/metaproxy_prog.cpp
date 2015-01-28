/* This file is part of Metaproxy.
   Copyright (C) Index Data

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
#include <yaz/backtrace.h>
#include <iostream>
#include <stdexcept>
#include <libxml/xinclude.h>

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include <metaproxy/router_xml.hpp>

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

mp::RouterXML *routerp = 0;

static void set_log_prefix(void)
{
#if HAVE_UNISTD_H
    char str[80];

    sprintf(str, "%lld", (long long) getpid());
    yaz_log_init_prefix(str);
#endif
}

#if HAVE_UNISTD_H
static pid_t process_group = 0;
static int sig_received = 0;
static pid_t my_pid = 0;

static void sig_x_handler(int signo)
{
    if (sig_received)
        return;
    if ( getpid() != my_pid ) // can happen in unlikely cases
        return; // when the sig hits a child just after fork(), before it
                // clears its sighandler. MP-559.
    sig_received = signo;
    if (routerp)
        routerp->stop(signo);
    if (signo == SIGTERM)
        kill(-process_group, SIGTERM); /* kill all children processes as well */
}
#endif

static void work_common(void *data)
{
    set_log_prefix();
#if HAVE_UNISTD_H
    process_group = getpgid(0); // save process group ID
    my_pid = getpid();

    signal(SIGTERM, sig_x_handler);
    signal(SIGUSR1, sig_x_handler);
#endif
    routerp = (mp::RouterXML*) data;
    routerp->start();

    mp::Package pack;
    pack.router(*routerp).move();
    yaz_log(YLOG_LOG, "metaproxy stop");
    delete routerp;
    routerp = 0;
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

    yaz_enable_panic_backtrace(argv[0]);
    set_log_prefix();

    while ((ret = options("c{config}:Dh{help}l:m:p:tu:v:V{version}w:X",
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
                " -v level\n"
                " -c|--config f config filename\n"
                " -D            daemon and keepalive operation\n"
                " -l f          log file f\n"
                " -m logformat  log time format (strftime)\n"
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
        case 'm':
            yaz_log_time_format(arg);
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
        case 'v':
            yaz_log_init_level(yaz_log_mask_str(arg));
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

    yaz_log(YLOG_LOG, "metaproxy %s " VERSION
#ifdef VERSION_SHA1
                " " VERSION_SHA1
#endif
        , test_config ? "test" : "start"
            );

    xmlInitParser();
    LIBXML_TEST_VERSION

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
        mp::RouterXML *router =
            new mp::RouterXML(doc, test_config, wrbuf_cstr(base_path));
        if (!test_config)
        {

            yaz_sc_running(s);

            yaz_daemon("metaproxy", mode | YAZ_DAEMON_LOG_REOPEN,
                       mode == YAZ_DAEMON_DEBUG ? work_debug : work_normal,
                       router, pidfile, uid);
        }
        delete router;
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
    if (test_config)
        yaz_log(YLOG_LOG, "metaproxy test exit code %d", ret);
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

