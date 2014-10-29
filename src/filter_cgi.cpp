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
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <yaz/poll.h>
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
            std::string documentroot;
            void child(Z_HTTP_Request *, const CGI::Exec *);
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

void yf::CGI::Rep::child(Z_HTTP_Request *hreq, const CGI::Exec *it)
{
    const char *path_cstr = hreq->path;
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
    setenv("REQUEST_METHOD", hreq->method, 1);
    setenv("REQUEST_URI", path_cstr, 1);
    setenv("SCRIPT_NAME", script_name.c_str(), 1);
    setenv("PATH_INFO", path_info.c_str(), 1);
    setenv("QUERY_STRING", query_string.c_str(), 1);
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
    setenv("DOCUMENT_ROOT", documentroot.c_str(), 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);

    v = z_HTTP_header_lookup(hreq->headers, "Content-Type");
    if (v)
    {
        char tmp[40];
        sprintf(tmp, "%d", hreq->content_len);
        setenv("CONTENT_LENGTH", tmp, 1);
        setenv("CONTENT_TYPE", v, 1);
    }
    // apply user-defined environment
    std::map<std::string,std::string>::const_iterator it_e;
    for (it_e = env_map.begin();
         it_e != env_map.end(); it_e++)
        setenv(it_e->first.c_str(), it_e->second.c_str(), 1);
    // change directory to configuration root
    // then to CGI program directory (could be relative)
    chdir(documentroot.c_str());
    char *program = xstrdup(program_cstr);
    char *cp = strrchr(program, '/');
    if (cp)
    {
        *cp++ = '\0';
        chdir(program);
    }
    else
        cp = program;
    int r = execl(cp, cp, (char *) 0);
    if (r == -1)
        exit(1);
    exit(0);
}

void yf::CGI::process(mp::Package &package) const
{
    Z_GDU *zgdu_req = package.request().get();
    Z_GDU *zgdu_res = 0;

    if (!zgdu_req || zgdu_req->which != Z_GDU_HTTP_Request)
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
            int fds_response[2];
            int r = pipe(fds_response);
            if (r == -1)
            {
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), hreq, 400);
                package.response() = zgdu_res;
                continue;
            }
            int fds_request[2];
            r = pipe(fds_request);
            if (r == -1)
            {
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), hreq, 400);
                package.response() = zgdu_res;
                close(fds_response[0]);
                close(fds_response[1]);
                continue;
            }

            int status;
            pid_t pid = ::fork();
            switch (pid)
            {
            case 0: /* child */
                /* POSTed content */
                close(0);
                dup(fds_request[0]);
                close(fds_request[1]);
                /* response */
                close(1);
                close(fds_response[0]);
                dup(fds_response[1]);
                m_p->child(hreq, &(*it));
                break;
            case -1: /* error */
                close(fds_request[0]);
                close(fds_request[1]);
                close(fds_response[0]);
                close(fds_response[1]);
                zgdu_res = odr.create_HTTP_Response(
                    package.session(), hreq, 400);
                package.response() = zgdu_res;
                break;
            default: /* parent */
                close(fds_response[1]);
                close(fds_request[0]);
                if (pid)
                {
                    boost::mutex::scoped_lock lock(m_p->m_mutex);
                    m_p->children[pid] = pid;
                }
                WRBUF w = wrbuf_alloc();
                wrbuf_puts(w, "HTTP/1.1 200 OK\r\n");
                fcntl(fds_response[0], F_SETFL, O_NONBLOCK);
                fcntl(fds_request[1], F_SETFL, O_NONBLOCK);
                int no_write = 0;
                while (1)
                {
                    int num = 1;
                    struct yaz_poll_fd fds[2];
                    fds[0].fd = fds_response[0];
                    fds[0].input_mask = yaz_poll_read;
                    if (no_write < hreq->content_len)
                    {
                        fds[1].fd = fds_request[1];
                        fds[1].input_mask = yaz_poll_write;
                        num = 2;
                    }
                    int r = yaz_poll(fds, num, 60, 0);
                    if (r <= 0)
                        break;
                    if (fds[0].output_mask & (yaz_poll_read|yaz_poll_except))
                    {
                        char buf[512];
                        ssize_t rd = read(fds_response[0], buf, sizeof buf);
                        if (rd <= 0)
                            break;
                        wrbuf_write(w, buf, rd);
                    }
                    if (num == 2 && fds[1].output_mask & yaz_poll_write)
                    {
                        ssize_t wd = write(fds_request[1],
                                           hreq->content_buf + no_write,
                                           hreq->content_len - no_write);
                        if (wd <= 0)
                            break;
                        no_write += wd;
                    }
                }
                close(fds_request[1]);
                close(fds_response[0]);
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
        else if (!strcmp((const char *) ptr->name, "documentroot"))
        {
            m_p->documentroot = path;
        }
        else
        {
            throw mp::filter::FilterException("Bad element "
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
    if (m_p->documentroot.length() == 0)
        m_p->documentroot = ".";
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

