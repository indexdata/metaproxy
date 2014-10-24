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
#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include <yaz/url.h>
#include "filter_http_client.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <boost/thread/mutex.hpp>

#include <list>
#include <map>
#include <iostream>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class HTTPClient::Rep {
            friend class HTTPClient;
            void proxy(mp::Package &package);
            std::string proxy_host;
            std::string default_host;
            int max_redirects;
            bool x_forwarded_for;
            bool bind_host;
            Rep();
        };
    }
}

yf::HTTPClient::Rep::Rep()
{
    max_redirects = 0;
    x_forwarded_for = false;
    bind_host = false;
}

yf::HTTPClient::HTTPClient() : m_p(new Rep)
{
}

yf::HTTPClient::~HTTPClient()
{
}

void yf::HTTPClient::Rep::proxy(mp::Package &package)
{
    Z_GDU *req_gdu = package.request().get();
    if (req_gdu && req_gdu->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request *hreq = req_gdu->u.HTTP_Request;
        Z_GDU *res_gdu = 0;
        mp::odr o;
        yaz_url_t yaz_url = yaz_url_create();
        const char *http_proxy =
            z_HTTP_header_remove(&hreq->headers, "X-Metaproxy-Proxy");

        if (!http_proxy)
            http_proxy = proxy_host.c_str();

        if (*http_proxy)
            yaz_url_set_proxy(yaz_url, http_proxy);

        yaz_url_set_max_redirects(yaz_url, max_redirects);

        if (x_forwarded_for)
        {
            std::string peer_name2 = package.origin().get_address();
            const char *peer_name1 =
                z_HTTP_header_lookup(hreq->headers, "X-Forwarded-For");
            std::string pcomb;
            if (peer_name1)
            {
                pcomb.append(peer_name1);
                pcomb.append(", ");
            }
            pcomb.append(peer_name2);
            z_HTTP_header_set(o, &hreq->headers, "X-Forwarded-For",
                              pcomb.c_str());
        }
        std::string uri;
        if (hreq->path[0] == '/')
        {
            if (default_host.length())
                uri = default_host + hreq->path;
        }
        else
            uri = hreq->path;


        if (bind_host)
        {
            std::string host = package.origin().get_bind_address();
            uri.append(" ");
            uri.append(host);
        }
        Z_HTTP_Response *http_response = 0;
        if (uri.length())
            http_response =
            yaz_url_exec(yaz_url, uri.c_str(), hreq->method,
                         hreq->headers, hreq->content_buf,
                         hreq->content_len);
        if (http_response)
        {
            res_gdu = o.create_HTTP_Response(package.session(), hreq, 200);
            z_HTTP_header_remove(&http_response->headers, "Transfer-Encoding");
            res_gdu->u.HTTP_Response = http_response;
        }
        else
        {
            res_gdu = o.create_HTTP_Response_details(
                package.session(),
                hreq, 502,
                yaz_url_get_error(yaz_url));
        }
        package.response() = res_gdu;
        yaz_url_destroy(yaz_url);
    }
    else
        package.move();
}

void yf::HTTPClient::process(mp::Package &package) const
{
    Z_GDU *gdu = package.request().get();
    if (gdu && gdu->which == Z_GDU_HTTP_Request)
        m_p->proxy(package);
    else
        package.move();
}

void mp::filter::HTTPClient::configure(const xmlNode * ptr, bool test_only,
                                       const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        else if (!strcmp((const char *) ptr->name, "proxy"))
        {
            m_p->proxy_host = mp::xml::get_text(ptr);
        }
        else if (!strcmp((const char *) ptr->name, "max-redirects"))
        {
            m_p->max_redirects = mp::xml::get_int(ptr, 0);
        }
        else if (!strcmp((const char *) ptr->name, "default-host"))
        {
            m_p->default_host = mp::xml::get_text(ptr);
            if (m_p->default_host.find("://") == std::string::npos)
            {
                throw mp::filter::FilterException
                    ("default-host is missing method (such as http://)"
                     " in http_client filter");
            }
        }
        else if (!strcmp((const char *) ptr->name, "x-forwarded-for"))
        {
            m_p->x_forwarded_for = mp::xml::get_bool(ptr, 0);
        }
        else if (!strcmp((const char *) ptr->name, "bind_host"))
        {
            m_p->bind_host = mp::xml::get_bool(ptr, 0);
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element "
                 + std::string((const char *) ptr->name)
                 + " in http_client filter");
        }
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::HTTPClient;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_http_client = {
        0,
        "http_client",
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

