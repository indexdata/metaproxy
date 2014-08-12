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

#include "filter_cgi.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include "gduutil.hpp"
#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sstream>

#include "config.hpp"

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class CGI::Exec {
            friend class Rep;
            friend class CGI;
            std::string path;
            std::string program;
        };
        class CGI::Rep {
            friend class CGI;
            std::list<CGI::Exec> exec_map;
            std::map<pid_t,pid_t> children;
            boost::mutex m_mutex;
        public:
            ~Rep();
        };
    }
}

yf::CGI::CGI() : m_p(new Rep)
{

}

yf::CGI::Rep::~Rep()
{
    std::map<pid_t,pid_t>::const_iterator it;
    boost::mutex::scoped_lock lock(m_mutex);

    for (it = children.begin(); it != children.end(); it++)
        kill(it->second, SIGTERM);
}

yf::CGI::~CGI()
{
}

void yf::CGI::process(mp::Package &package) const
{
    Z_GDU *zgdu_req = package.request().get();
    Z_GDU *zgdu_res = 0;

    if (!zgdu_req)
        return;

    if (zgdu_req->which != Z_GDU_HTTP_Request)
    {
        package.move();
        return;
    }
    std::string path_info;
    std::string query_string;
    const char *path = zgdu_req->u.HTTP_Request->path;
    yaz_log(YLOG_LOG, "path=%s", path);
    const char *p_cp = strchr(path, '?');
    if (p_cp)
    {
        path_info.assign(path, p_cp - path);
        query_string.assign(p_cp+1);
    }
    else
        path_info.assign(path);

    std::list<CGI::Exec>::const_iterator it;
    metaproxy_1::odr odr;
    for (it = m_p->exec_map.begin(); it != m_p->exec_map.end(); it++)
    {
        if (it->path.compare(path_info) == 0)
        {
            int r;
            pid_t pid;
            int status;
            int fds[2];

            r = pipe(fds);
            if (r == -1)
            {
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), zgdu_req->u.HTTP_Request, 400);
                package.response() = zgdu_res;
                continue;
            }
            pid = ::fork();
            switch (pid)
            {
            case 0: /* child */
                close(1);
                dup(fds[1]);
                setenv("PATH_INFO", path_info.c_str(), 1);
                setenv("QUERY_STRING", query_string.c_str(), 1);
                r = execl(it->program.c_str(), it->program.c_str(), (char *) 0);
                if (r == -1)
                    exit(1);
                exit(0);
                break;
            case -1: /* error */
                close(fds[0]);
                close(fds[1]);
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), zgdu_req->u.HTTP_Request, 400);
                package.response() = zgdu_res;
                break;
            default: /* parent */
                close(fds[1]);
                if (pid)
                {
                    boost::mutex::scoped_lock lock(m_p->m_mutex);
                    m_p->children[pid] = pid;
                }
                WRBUF w = wrbuf_alloc();
                wrbuf_puts(w, "HTTP/1.1 200 OK\r\n");
                while (1)
                {
                    char buf[512];
                    ssize_t rd = read(fds[0], buf, sizeof buf);
                    if (rd <= 0)
                        break;
                    wrbuf_write(w, buf, rd);
                }
                close(fds[0]);
                waitpid(pid, &status, 0);

                if (pid)
                {
                    boost::mutex::scoped_lock lock(m_p->m_mutex);
                    m_p->children.erase(pid);
                }
                ODR dec = odr_createmem(ODR_DECODE);
                odr_setbuf(dec, wrbuf_buf(w), wrbuf_len(w), 0);
                r = z_GDU(dec, &zgdu_res, 0, 0);
                if (r && zgdu_res)
                {
                    package.response() = zgdu_res;
                }
                else
                {
                    zgdu_res = odr.create_HTTP_Response(
                        package.session(), zgdu_req->u.HTTP_Request, 400);
                    Z_HTTP_Response *hres = zgdu_res->u.HTTP_Response;
                    z_HTTP_header_add(odr, &hres->headers,
                                      "Content-Type", "text/plain");
                    hres->content_buf =
                        odr_strdup(odr, "Invalid script from script");
                    hres->content_len = strlen(hres->content_buf);
                }
                package.response() = zgdu_res;
                odr_destroy(dec);
                wrbuf_destroy(w);
                break;
            }
            return;
        }
    }
    package.move();
}

void yf::CGI::configure(const xmlNode *ptr, bool test_only, const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "map"))
        {
            CGI::Exec exec;

            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name,  "path"))
                    exec.path = mp::xml::get_text(attr->children);
                else if (!strcmp((const char *) attr->name, "exec"))
                    exec.program = mp::xml::get_text(attr->children);
                else
                    throw mp::filter::FilterException
                        ("Bad attribute "
                         + std::string((const char *) attr->name)
                         + " in cgi section");
            }
            m_p->exec_map.push_back(exec);
        }
        else
        {
            throw mp::filter::FilterException("Bad element "
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::CGI;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_cgi = {
        0,
        "cgi",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

