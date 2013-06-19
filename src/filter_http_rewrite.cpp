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
#include "filter_http_rewrite.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <map>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class HttpRewrite::RuleScope {
        public:
            std::vector<std::string> tags;
            std::vector<std::string> attrs;
            std::string content_type;
        };
        class HttpRewrite::Rule {
        public:
            enum Section { METHOD, HEADER, BODY };
            std::string regex;
            std::string recipe;
            std::map<int, std::string> group_index;
            std::vector<RuleScope> scopes;
            Section section;
            const std::string search_replace(
                std::map<std::string, std::string> & vars,
                const std::string & txt) const;
            std::string sub_vars (
                const std::map<std::string, std::string> & vars) const;
            void parse_groups();
        };
        class HttpRewrite::Rules {
        public:
            std::vector<Rule> rules;
            void rewrite_reqline (mp::odr & o, Z_HTTP_Request *hreq,
                std::map<std::string, std::string> & vars) const;
            void rewrite_headers(mp::odr & o, Z_HTTP_Header *headers,
                std::map<std::string, std::string> & vars) const;
            void rewrite_body (mp::odr & o,
                char **content_buf, int *content_len,
                std::map<std::string, std::string> & vars) const;
            const std::string test_patterns(
                std::map<std::string, std::string> & vars,
                const std::string & txt) const;
        };
    }
}

yf::HttpRewrite::HttpRewrite() : req_rules(new Rules), res_rules(new Rules)
{
}

yf::HttpRewrite::~HttpRewrite()
{
}

void yf::HttpRewrite::process(mp::Package & package) const
{
    yaz_log(YLOG_LOG, "HttpRewrite begins....");
    Z_GDU *gdu = package.request().get();
    //map of request/response vars
    std::map<std::string, std::string> vars;
    //we have an http req
    if (gdu && gdu->which == Z_GDU_HTTP_Request)
    {
        Z_HTTP_Request *hreq = gdu->u.HTTP_Request;
        mp::odr o;
        req_rules->rewrite_reqline(o, hreq, vars);
        yaz_log(YLOG_LOG, ">> Request headers");
        req_rules->rewrite_headers(o, hreq->headers, vars);
        req_rules->rewrite_body(o,
                &hreq->content_buf, &hreq->content_len,
                vars);
        package.request() = gdu;
    }
    package.move();
    gdu = package.response().get();
    if (gdu && gdu->which == Z_GDU_HTTP_Response)
    {
        Z_HTTP_Response *hres = gdu->u.HTTP_Response;
        yaz_log(YLOG_LOG, "Response code %d", hres->code);
        mp::odr o;
        yaz_log(YLOG_LOG, "<< Respose headers");
        res_rules->rewrite_headers(o, hres->headers, vars);
        res_rules->rewrite_body(o, &hres->content_buf,
                &hres->content_len, vars);
        package.response() = gdu;
    }
}

void yf::HttpRewrite::Rules::rewrite_reqline (mp::odr & o,
        Z_HTTP_Request *hreq,
        std::map<std::string, std::string> & vars) const
{
    //rewrite the request line
    std::string path;
    if (strstr(hreq->path, "http://") == hreq->path)
    {
        yaz_log(YLOG_LOG, "Path in the method line is absolute, "
            "possibly a proxy request");
        path += hreq->path;
    }
    else
    {
        //TODO what about proto
        path += "http://";
        path += z_HTTP_header_lookup(hreq->headers, "Host");
        path += hreq->path;
    }
    yaz_log(YLOG_LOG, "Proxy request URL is %s", path.c_str());
    std::string npath =
        test_patterns(vars, path);
    if (!npath.empty())
    {
        yaz_log(YLOG_LOG, "Rewritten request URL is %s", npath.c_str());
        hreq->path = odr_strdup(o, npath.c_str());
    }
}

void yf::HttpRewrite::Rules::rewrite_headers(mp::odr & o,
        Z_HTTP_Header *headers,
        std::map<std::string, std::string> & vars) const
{
    for (Z_HTTP_Header *header = headers;
            header != 0;
            header = header->next)
    {
        std::string sheader(header->name);
        sheader += ": ";
        sheader += header->value;
        yaz_log(YLOG_LOG, "%s: %s", header->name, header->value);
        std::string out = test_patterns(vars, sheader);
        if (!out.empty())
        {
            size_t pos = out.find(": ");
            if (pos == std::string::npos)
            {
                yaz_log(YLOG_LOG, "Header malformed during rewrite, ignoring");
                continue;
            }
            header->name = odr_strdup(o, out.substr(0, pos).c_str());
            header->value = odr_strdup(o, out.substr(pos+2,
                        std::string::npos).c_str());
        }
    }
}

void yf::HttpRewrite::Rules::rewrite_body (mp::odr & o,
        char **content_buf,
        int *content_len,
        std::map<std::string, std::string> & vars) const
{
    if (*content_buf)
    {
        std::string body(*content_buf);
        std::string nbody =
            test_patterns(vars, body);
        if (!nbody.empty())
        {
            *content_buf = odr_strdup(o, nbody.c_str());
            *content_len = nbody.size();
        }
    }
}

/**
 * Tests pattern from the vector in order and executes recipe on
 the first match.
 */
const std::string yf::HttpRewrite::Rules::test_patterns(
        std::map<std::string, std::string> & vars,
        const std::string & txt) const
{
    for (size_t i = 0; i < rules.size(); i++)
    {
        std::string out = rules[i].search_replace(vars, txt);
        if (!out.empty()) return out;
    }
    return "";
}

const std::string yf::HttpRewrite::Rule::search_replace(
        std::map<std::string, std::string> & vars,
        const std::string & txt) const
{
    //exec regex against value
    boost::regex re(regex);
    boost::smatch what;
    std::string::const_iterator start, end;
    start = txt.begin();
    end = txt.end();
    std::string out;
    while (regex_search(start, end, what, re)) //find next full match
    {
        size_t i;
        for (i = 1; i < what.size(); ++i)
        {
            //check if the group is named
            std::map<int, std::string>::const_iterator it
                = group_index.find(i);
            if (it != group_index.end())
            {   //it is
                if (!what[i].str().empty())
                    vars[it->second] = what[i];
            }

        }
        //prepare replacement string
        std::string rvalue = sub_vars(vars);
        yaz_log(YLOG_LOG, "! Rewritten '%s' to '%s'",
                what.str(0).c_str(), rvalue.c_str());
        out.append(start, what[0].first);
        out.append(rvalue);
        start = what[0].second; //move search forward
    }
    //if we had a match cat the last part
    if (start != txt.begin())
        out.append(start, end);
    return out;
}

void yf::HttpRewrite::Rule::parse_groups()
{
    int gnum = 0;
    bool esc = false;
    const std::string & str = regex;
    std::string res;
    yaz_log(YLOG_LOG, "Parsing groups from '%s'", str.c_str());
    for (size_t i = 0; i < str.size(); ++i)
    {
        res += str[i];
        if (!esc && str[i] == '\\')
        {
            esc = true;
            continue;
        }
        if (!esc && str[i] == '(') //group starts
        {
            gnum++;
            if (i+1 < str.size() && str[i+1] == '?') //group with attrs
            {
                i++;
                if (i+1 < str.size() && str[i+1] == ':') //non-capturing
                {
                    if (gnum > 0) gnum--;
                    res += str[i];
                    i++;
                    res += str[i];
                    continue;
                }
                if (i+1 < str.size() && str[i+1] == 'P') //optional, python
                    i++;
                if (i+1 < str.size() && str[i+1] == '<') //named
                {
                    i++;
                    std::string gname;
                    bool term = false;
                    while (++i < str.size())
                    {
                        if (str[i] == '>') { term = true; break; }
                        if (!isalnum(str[i]))
                            throw mp::filter::FilterException
                                ("Only alphanumeric chars allowed, found "
                                 " in '"
                                 + str
                                 + "' at "
                                 + boost::lexical_cast<std::string>(i));
                        gname += str[i];
                    }
                    if (!term)
                        throw mp::filter::FilterException
                            ("Unterminated group name '" + gname
                             + " in '" + str +"'");
                    group_index[gnum] = gname;
                    yaz_log(YLOG_LOG, "Found named group '%s' at $%d",
                            gname.c_str(), gnum);
                }
            }
        }
        esc = false;
    }
    regex = res;
}

std::string yf::HttpRewrite::Rule::sub_vars (
        const std::map<std::string, std::string> & vars) const
{
    std::string out;
    bool esc = false;
    const std::string & in = recipe;
    for (size_t i = 0; i < in.size(); ++i)
    {
        if (!esc && in[i] == '\\')
        {
            esc = true;
            continue;
        }
        if (!esc && in[i] == '$') //var
        {
            if (i+1 < in.size() && in[i+1] == '{') //ref prefix
            {
                ++i;
                std::string name;
                bool term = false;
                while (++i < in.size())
                {
                    if (in[i] == '}') { term = true; break; }
                    name += in[i];
                }
                if (!term) throw mp::filter::FilterException
                    ("Unterminated var ref in '"+in+"' at "
                     + boost::lexical_cast<std::string>(i));
                std::map<std::string, std::string>::const_iterator it
                    = vars.find(name);
                if (it != vars.end())
                {
                    out += it->second;
                }
            }
            else
            {
                throw mp::filter::FilterException
                    ("Malformed or trimmed var ref in '"
                     +in+"' at "+boost::lexical_cast<std::string>(i));
            }
            continue;
        }
        //passthru
        out += in[i];
        esc = false;
    }
    return out;
}

void yf::HttpRewrite::configure_rules(const xmlNode *ptr,
        Rules & rules)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        else if (!strcmp((const char *) ptr->name, "rewrite"))
        {
            Rule rule;
            const struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
            {
                if (!strcmp((const char *) attr->name,  "from"))
                    rule.regex = mp::xml::get_text(attr->children);
                else if (!strcmp((const char *) attr->name,  "to"))
                    rule.recipe = mp::xml::get_text(attr->children);
                else
                    throw mp::filter::FilterException
                        ("Bad attribute "
                         + std::string((const char *) attr->name)
                         + " in rewrite section of http_rewrite");
            }
            yaz_log(YLOG_LOG, "Found rewrite rule from '%s' to '%s'",
                    rule.regex.c_str(), rule.recipe.c_str());
            rule.parse_groups();
            if (!rule.regex.empty())
                rules.rules.push_back(rule);
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element o"
                 + std::string((const char *) ptr->name)
                 + " in http_rewrite1 filter");
        }
    }
}

void yf::HttpRewrite::configure(const xmlNode * ptr, bool test_only,
        const char *path)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        else if (!strcmp((const char *) ptr->name, "request"))
        {
            configure_rules(ptr, *req_rules);
        }
        else if (!strcmp((const char *) ptr->name, "response"))
        {
            configure_rules(ptr, *res_rules);
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
    return new mp::filter::HttpRewrite;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_http_rewrite = {
        0,
        "http_rewrite",
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

