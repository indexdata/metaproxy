/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

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
#include "filter_http_rewrite1.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <boost/thread/mutex.hpp>
#include <boost/regex.hpp>

#include <list>
#include <map>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class HttpRewrite1::Rule {
        public:
            std::string content_type;
            std::string pattern;
            std::string replacement;
            std::string mode;
        };
        class HttpRewrite1::Rep {
            friend class HttpRewrite1;
            void rewrite_response(mp::odr &o, Z_HTTP_Response *hres);
            std::list<Rule> rules;
        };
    }
}

yf::HttpRewrite1::HttpRewrite1() : m_p(new Rep)
{
}

yf::HttpRewrite1::~HttpRewrite1()
{
}

void yf::HttpRewrite1::Rep::rewrite_response(mp::odr &o, Z_HTTP_Response *hres)
{
    const char *ctype = z_HTTP_header_lookup(hres->headers, "Content-Type");
    if (ctype && hres->content_buf)
    {
        std::string text(hres->content_buf, hres->content_len);
        std::list<Rule>::const_iterator it;
        int number_of_replaces = 0;
        for (it = rules.begin(); it != rules.end(); it++)
        {
            if (strcmp(ctype, it->content_type.c_str()) == 0)
            {
                boost::regex::flag_type b_mode = boost::regex::perl;
                if (it->mode.find_first_of('i') != std::string::npos)
                    b_mode |= boost::regex::icase;
                boost::regex e(it->pattern, b_mode);
                boost::match_flag_type match_mode = boost::format_first_only;
                if (it->mode.find_first_of('g') != std::string::npos)
                    match_mode = boost::format_all;
                text = regex_replace(text, e, it->replacement, match_mode);
                number_of_replaces++;
            }
        }
        if (number_of_replaces > 0)
        {
            hres->content_buf = odr_strdup(o, text.c_str());
            hres->content_len = strlen(hres->content_buf);
        }
    }
}

void yf::HttpRewrite1::process(mp::Package &package) const
{
    Z_GDU *gdu_req = package.request().get();
    if (gdu_req && gdu_req->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request *hreq = gdu_req->u.HTTP_Request; 

        assert(hreq); // not changing request (such as POST content)
        package.move();

        Z_GDU *gdu_res = package.response().get();
        Z_HTTP_Response *hres = gdu_res->u.HTTP_Response;
        if (hres)
        {
            mp::odr o;
            m_p->rewrite_response(o, hres);
            package.response() = gdu_res;
        }
    }
    else
        package.move();
}

void mp::filter::HttpRewrite1::configure(const xmlNode * ptr, bool test_only,
                                     const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        else if (!strcmp((const char *) ptr->name, "replace"))
        {
            HttpRewrite1::Rule rule;

            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name,  "pattern"))
                    rule.pattern = mp::xml::get_text(attr->children);
                else if (!strcmp((const char *) attr->name,  "replacement"))
                    rule.replacement = mp::xml::get_text(attr->children);
                else if (!strcmp((const char *) attr->name,  "mode"))
                    rule.mode = mp::xml::get_text(attr->children);
                else if (!strcmp((const char *) attr->name, "content-type"))
                    rule.content_type = mp::xml::get_text(attr->children);
                else
                    throw mp::filter::FilterException
                        ("Bad attribute "
                         + std::string((const char *) attr->name)
                         + " in replace section of http_rewrite1");
            }
            if (rule.pattern.length() > 0)
                m_p->rules.push_back(rule);
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element "
                 + std::string((const char *) ptr->name)
                 + " in http_rewrite1 filter");
        }
    }
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::HttpRewrite1;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_http_rewrite1 = {
        0,
        "http_rewrite1",
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

