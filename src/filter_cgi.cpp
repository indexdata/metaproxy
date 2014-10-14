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
            std::map<std::string,std::string> env_map;
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


    std::list<CGI::Exec>::const_iterator it;
    metaproxy_1::odr odr;
    Z_HTTP_Request *hreq = zgdu_req->u.HTTP_Request;
    const char *path_cstr = hreq->path;
    for (it = m_p->exec_map.begin(); it != m_p->exec_map.end(); it++)
    {
        if (strncmp(it->path.c_str(), path_cstr, it->path.length()) == 0)
        {
            std::string path(path_cstr);
            const char *program_cstr = it->program.c_str();
            std::string script_name(path, 0, it->path.length());
            std::string rest(path, it->path.length());
            std::string query_string;
            std::string path_info;
            size_t qpos = rest.find('?');
            if (qpos == std::string::npos)
                path_info = rest;
            else
            {
                query_string.assign(rest, qpos + 1, std::string::npos);
                path_info.assign(rest, 0, qpos);
            }
            int fds[2];
            int r = pipe(fds);
            if (r == -1)
            {
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), hreq, 400);
                package.response() = zgdu_res;
                continue;
            }
            int status;
            pid_t pid = ::fork();
            switch (pid)
            {
            case 0: /* child */
                close(1);
                dup(fds[1]);
                setenv("REQUEST_METHOD", hreq->method, 1);
                setenv("REQUEST_URI", path_cstr, 1);
                setenv("SCRIPT_NAME", script_name.c_str(), 1);
                setenv("PATH_INFO", path_info.c_str(), 1);
                setenv("QUERY_STRING", query_string.c_str(), 1);
                {
                    const char *v;
                    v = z_HTTP_header_lookup(hreq->headers, "Cookie");
                    if (v)
                        setenv("HTTP_COOKIE", v, 1);
                    v = z_HTTP_header_lookup(hreq->headers, "User-Agent");
                    if (v)
                        setenv("HTTP_USER_AGENT", v, 1);
                    v = z_HTTP_header_lookup(hreq->headers, "Accept");
                    if (v)
                        setenv("HTTP_ACCEPT", v, 1);
                    v = z_HTTP_header_lookup(hreq->headers, "Accept-Encoding");
                    if (v)
                        setenv("HTTP_ACCEPT_ENCODING", v, 1);
                    std::map<std::string,std::string>::const_iterator it;
                    for (it = m_p->env_map.begin();
                         it != m_p->env_map.end(); it++)
                        setenv(it->first.c_str(), it->second.c_str(), 1);
                    char *program = xstrdup(program_cstr);
                    char *cp = strrchr(program, '/');
                    if (cp)
                    {
                        *cp++ = '\0';
                        chdir(program);
                    }
                    else
                        cp = program;
                    r = execl(cp, cp, (char *) 0);
                }
                if (r == -1)
                    exit(1);
                exit(0);
                break;
            case -1: /* error */
                close(fds[0]);
                close(fds[1]);
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), hreq, 400);
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
    yaz_log(YLOG_LOG, "cgi::configure path=%s", path);
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
        else if (!strcmp((const char *) ptr->name, "env"))
        {
            std::string name, value;

            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name,  "name"))
                    name = mp::xml::get_text(attr->children);
                else if (!strcmp((const char *) attr->name, "value"))
                    value = mp::xml::get_text(attr->children);
                else
                    throw mp::filter::FilterException
                        ("Bad attribute "
                         + std::string((const char *) attr->name)
                         + " in cgi section");
            }
            if (name.length() > 0)
                m_p->env_map[name] = value;
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

