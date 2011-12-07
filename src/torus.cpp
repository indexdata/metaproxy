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
#include <yaz/log.h>
#include <yaz/url.h>
#include <metaproxy/util.hpp>
#include "torus.hpp"

namespace mp = metaproxy_1;

xmlDoc *mp::get_searchable(mp::Package &package,
                           std::string url_template, const std::string &db,
                           const std::string &realm,
                           const std::string &proxy)
{
    // http://newmk2.indexdata.com/torus2/searchable.ebsco/records/?query=udb=aberdeenUni
    xmlDoc *doc = 0;
    size_t found;

    found = url_template.find("%db");
    if (found != std::string::npos)
        url_template.replace(found, 3, mp::util::uri_encode(db));

    found = url_template.find("%realm");
    if (found != std::string::npos)
        url_template.replace(found, 6, mp::util::uri_encode(realm));

    Z_HTTP_Header *http_headers = 0;
    mp::odr odr;
    
    z_HTTP_header_add(odr, &http_headers, "Accept","application/xml");

    yaz_url_t url_p = yaz_url_create();
    if (proxy.length())
        yaz_url_set_proxy(url_p, proxy.c_str());

    Z_HTTP_Response *http_response = yaz_url_exec(url_p,
                                                  url_template.c_str(),
                                                  "GET",
                                                  http_headers,
                                                  0, /* content buf */
                                                  0  /* content_len */
        );
    if (http_response && http_response->code == 200 && 
        http_response->content_buf)
    {
        package.log("zoom", YLOG_LOG, "Torus: %s OK", url_template.c_str());
        doc = xmlParseMemory(http_response->content_buf,
                             http_response->content_len);
        
    }
    else
    {
        package.log("zoom", YLOG_WARN, "Torus: %s FAIL", url_template.c_str());
        if (http_response)
        {
            package.log("zoom", YLOG_LOG, "HTTP code: %d", http_response->code);
        }
    }

    if (http_response && http_response->content_buf)
    {
        package.log("zoom", YLOG_LOG, "HTTP content");
        package.log_write(http_response->content_buf,
                          http_response->content_len);
    }
    yaz_url_destroy(url_p);
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

