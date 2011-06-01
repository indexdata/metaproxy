/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

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

#include <metaproxy/xmlutil.hpp>

#include <string.h>
#include <yaz/wrbuf.h>
#include <yaz/zgdu.h>
#include <yaz/srw.h>
#include <yaz/comstack.h>

#include "torus.hpp"

namespace mp = metaproxy_1;


static Z_GDU *get_HTTP_Request_url(ODR odr, const char *url)
{
    Z_GDU *p = z_get_HTTP_Request(odr);
    const char *host = url;
    const char *cp0 = strstr(host, "://");
    const char *cp1 = 0;
    if (cp0)
        cp0 = cp0+3;
    else
        cp0 = host;
    
    cp1 = strchr(cp0, '/');
    if (!cp1)
        cp1 = cp0 + strlen(cp0);
    
    if (cp0 && cp1)
    {
        char *h = (char*) odr_malloc(odr, cp1 - cp0 + 1);
        memcpy (h, cp0, cp1 - cp0);
        h[cp1-cp0] = '\0';
        z_HTTP_header_add(odr, &p->u.HTTP_Request->headers, "Host", h);
    }
    p->u.HTTP_Request->path = odr_strdup(odr, *cp1 ? cp1 : "/");
    return p;
}

static WRBUF get_url(const char *uri, WRBUF username, WRBUF password,
                     int *code)
{
    int number_of_redirects = 0;
    WRBUF result = 0;
    ODR out = odr_createmem(ODR_ENCODE);
    ODR in = odr_createmem(ODR_DECODE);

    while (1)
    {
        Z_HTTP_Response *res = 0;
        const char *location = 0;
        Z_GDU *gdu = get_HTTP_Request_url(out, uri);
        yaz_log(YLOG_LOG, "GET %s", uri);
        gdu->u.HTTP_Request->method = odr_strdup(out, "GET");
        if (username && password)
        {
            z_HTTP_header_add_basic_auth(out, &gdu->u.HTTP_Request->headers,
                                         wrbuf_cstr(username),
                                         wrbuf_cstr(password));
        }
        z_HTTP_header_add(out, &gdu->u.HTTP_Request->headers, "Accept",
                          "application/xml");
        if (!z_GDU(out, &gdu, 0, 0))
        {
            yaz_log(YLOG_WARN, "Can not encode HTTP request URL:%s", uri);        
            break;
        }
        void *add;
        COMSTACK conn = cs_create_host(uri, 1, &add);
        if (!conn)
            yaz_log(YLOG_WARN, "Bad address for URL:%s", uri);
        else if (cs_connect(conn, add) < 0)
            yaz_log(YLOG_WARN, "Can not connect to URL:%s", uri);
        else
        {
            int len;
            char *buf = odr_getbuf(out, &len, 0);
            
            if (cs_put(conn, buf, len) < 0)
                yaz_log(YLOG_WARN, "cs_put failed URL:%s", uri);
            else
            {
                char *netbuffer = 0;
                int netlen = 0;
                int cs_res = cs_get(conn, &netbuffer, &netlen);
                if (cs_res <= 0)
                {
                    yaz_log(YLOG_WARN, "cs_get failed URL:%s", uri);
                }
                else
                {
                    Z_GDU *gdu;
                    odr_setbuf(in, netbuffer, cs_res, 0);
                    if (!z_GDU(in, &gdu, 0, 0)
                        || gdu->which != Z_GDU_HTTP_Response)
                    {
                        yaz_log(YLOG_WARN, "HTTP decoding failed "
                                "URL:%s", uri);
                    }
                    else
                    {
                        res = gdu->u.HTTP_Response;
                    }
                }
                xfree(netbuffer);
            }
            cs_close(conn);
        }
        if (!res)
            break; // ERROR
        *code = res->code;
        location = z_HTTP_header_lookup(res->headers, "Location");
        if (++number_of_redirects < 10 &&
            location && (*code == 301 || *code == 302 || *code == 307))
        {
            odr_reset(out);
            uri = odr_strdup(out, location);
            odr_reset(in);
        }
        else
        {
            result = wrbuf_alloc();
            wrbuf_write(result, res->content_buf, res->content_len);
            break;
        }
    }
    odr_destroy(out);
    odr_destroy(in);
    return result;
}


mp::Torus::Torus()
{
    doc = 0;
}

mp::Torus::~Torus()
{
    if (doc)
        xmlFreeDoc(doc);
}

void mp::Torus::read_searchables(std::string url)
{
    if (doc)
    {
        xmlFreeDoc(doc);
        doc = 0;
    }
    if (url.length() == 0)
        return;

    int code;
    WRBUF w = get_url(url.c_str(), 0, 0, &code);
    if (code == 200)
    {
        doc = xmlParseMemory(wrbuf_buf(w), wrbuf_len(w));
        if (doc)
            yaz_log(YLOG_LOG, "xmlParseMemory OK");
    }
    wrbuf_destroy(w);
}

xmlDoc *mp::Torus::get_doc()
{
    return doc;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

