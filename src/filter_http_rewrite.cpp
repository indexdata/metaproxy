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
#include "html_parser.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <stack>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <map>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class HttpRewrite::Replace {
        public:
            bool start_anchor;
            boost::regex re;
            boost::smatch what;
            std::string recipe;
            std::map<int, std::string> group_index;
            std::string sub_vars(
                const std::map<std::string, std::string> & vars) const;
            void parse_groups(std::string pattern);
        };

        class HttpRewrite::Rule {
        public:
            std::list<Replace> replace_list;
            bool test_patterns(
                std::map<std::string, std::string> &vars,
                std::string &txt, bool anchor);
        };
        class HttpRewrite::Within {
        public:
            std::string header;
            std::string attr;
            boost::regex tag;
            std::string type;
            bool reqline;
            RulePtr rule;
            bool exec(std::map<std::string, std::string> &vars,
                      std::string &txt, bool anchor) const;
        };

        class HttpRewrite::Content {
        public:
            std::string type;
            boost::regex content_re;
            std::list<Within> within_list;
            void configure(const xmlNode *ptr,
                           std::map<std::string, RulePtr > &rules);
            void quoted_literal(std::string &content,
                                std::map<std::string, std::string> &vars) const;
            void parse(int verbose, std::string &content,
                       std::map<std::string, std::string> & vars) const;
        };
        class HttpRewrite::Phase {
        public:
            Phase();
            int m_verbose;
            std::list<Content> content_list;
            void rewrite_reqline(mp::odr & o, Z_HTTP_Request *hreq,
                std::map<std::string, std::string> & vars) const;
            void rewrite_headers(mp::odr & o, Z_HTTP_Header *headers,
                std::map<std::string, std::string> & vars) const;
            void rewrite_body(mp::odr & o,
                              const char *content_type,
                              char **content_buf, int *content_len,
                              std::map<std::string, std::string> & vars) const;
        };
        class HttpRewrite::Event : public HTMLParserEvent {
            void openTagStart(const char *tag, int tag_len);
            void anyTagEnd(const char *tag, int tag_len, int close_it);
            void attribute(const char *tag, int tag_len,
                           const char *attr, int attr_len,
                           const char *value, int val_len,
                           const char *sep);
            void closeTag(const char *tag, int tag_len);
            void text(const char *value, int len);
            const Content *m_content;
            WRBUF m_w;
            std::stack<std::list<Within>::const_iterator> s_within;
            std::map<std::string, std::string> &m_vars;
        public:
            Event(const Content *p, std::map<std::string, std::string> &vars);
            ~Event();
            const char *result();
        };
    }
}

yf::HttpRewrite::HttpRewrite() :
    req_phase(new Phase), res_phase(new Phase)
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
        req_phase->rewrite_reqline(o, hreq, vars);
        yaz_log(YLOG_LOG, ">> Request headers");
        req_phase->rewrite_headers(o, hreq->headers, vars);
        req_phase->rewrite_body(o,
                                z_HTTP_header_lookup(hreq->headers,
                                                     "Content-Type"),
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
        res_phase->rewrite_headers(o, hres->headers, vars);
        res_phase->rewrite_body(o,
                                z_HTTP_header_lookup(hres->headers,
                                                     "Content-Type"),
                                &hres->content_buf, &hres->content_len,
                                vars);
        package.response() = gdu;
    }
}

void yf::HttpRewrite::Phase::rewrite_reqline (mp::odr & o,
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
        const char *host = z_HTTP_header_lookup(hreq->headers, "Host");
        if (!host)
            return;

        path += "http://";
        path += host;
        path += hreq->path;
    }

    std::list<Content>::const_iterator cit = content_list.begin();
    for (; cit != content_list.end(); cit++)
        if (cit->type == "headers")
            break;

    if (cit == content_list.end())
        return;

    std::list<Within>::const_iterator it = cit->within_list.begin();
    for (; it != cit->within_list.end(); it++)
        if (it->reqline)
        {
            yaz_log(YLOG_LOG, "Proxy request URL is %s", path.c_str());
            if (it->exec(vars, path, true))
            {
                yaz_log(YLOG_LOG, "Rewritten request URL is %s", path.c_str());
                hreq->path = odr_strdup(o, path.c_str());
            }
        }
}

void yf::HttpRewrite::Phase::rewrite_headers(mp::odr & o,
        Z_HTTP_Header *headers,
        std::map<std::string, std::string> & vars) const
{
    std::list<Content>::const_iterator cit = content_list.begin();
    for (; cit != content_list.end(); cit++)
        if (cit->type == "headers")
            break;

    if (cit == content_list.end())
        return;

    for (Z_HTTP_Header *header = headers; header; header = header->next)
    {
        std::list<Within>::const_iterator it = cit->within_list.begin();
        for (; it != cit->within_list.end(); it++)
        {
            if (it->header.length() > 0 &&
                yaz_strcasecmp(it->header.c_str(), header->name) == 0)
            {
                std::string sheader(header->name);
                sheader += ": ";
                sheader += header->value;

                if (it->exec(vars, sheader, true))
                {
                    size_t pos = sheader.find(": ");
                    if (pos == std::string::npos)
                    {
                        yaz_log(YLOG_LOG, "Header malformed during rewrite, ignoring");
                        continue;
                    }
                    header->name = odr_strdup(
                        o, sheader.substr(0, pos).c_str());
                    header->value = odr_strdup(
                        o, sheader.substr(pos + 2, std::string::npos).c_str());
                }
            }
        }
    }
}

void yf::HttpRewrite::Phase::rewrite_body(
    mp::odr &o,
    const char *content_type,
    char **content_buf,
    int *content_len,
    std::map<std::string, std::string> & vars) const
{
    if (*content_len == 0)
        return;
    std::list<Content>::const_iterator cit = content_list.begin();
    for (; cit != content_list.end(); cit++)
    {
        yaz_log(YLOG_LOG, "rewrite_body: content_type=%s type=%s",
                content_type, cit->type.c_str());
        if (cit->type != "headers"
            && regex_match(content_type, cit->content_re))
            break;
    }
    if (cit == content_list.end())
        return;

    int i;
    for (i = 0; i < *content_len; i++)
        if ((*content_buf)[i] == 0)
            return;  // binary content. skip

    std::string content(*content_buf, *content_len);
    cit->parse(m_verbose, content, vars);
    *content_buf = odr_strdup(o, content.c_str());
    *content_len = strlen(*content_buf);
}

yf::HttpRewrite::Event::Event(const Content *p,
                              std::map<std::string, std::string> & vars
    ) : m_content(p), m_vars(vars)
{
    m_w = wrbuf_alloc();
}

yf::HttpRewrite::Event::~Event()
{
    wrbuf_destroy(m_w);
}

const char *yf::HttpRewrite::Event::result()
{
    return wrbuf_cstr(m_w);
}

void yf::HttpRewrite::Event::openTagStart(const char *tag, int tag_len)
{
    wrbuf_putc(m_w, '<');
    wrbuf_write(m_w, tag, tag_len);

    std::string t(tag, tag_len);
    std::list<Within>::const_iterator it = m_content->within_list.begin();
    for (; it != m_content->within_list.end(); it++)
    {
        if (!it->tag.empty() && regex_match(t, it->tag))
        {
            std::vector<std::string> attr;
            boost::split(attr, it->attr, boost::is_any_of(","));
            size_t i;
            for (i = 0; i < attr.size(); i++)
            {
                if (attr[i].compare("#text") == 0)
                {
                    s_within.push(it);
                    return;
                }
            }
        }
    }
}

void yf::HttpRewrite::Event::anyTagEnd(const char *tag, int tag_len,
                                       int close_it)
{
    if (close_it)
    {
        if (!s_within.empty())
        {
            std::list<Within>::const_iterator it = s_within.top();
            std::string t(tag, tag_len);
            if (regex_match(t, it->tag))
                s_within.pop();
        }
    }
    if (close_it)
        wrbuf_putc(m_w, '/');
    wrbuf_putc(m_w, '>');
}

void yf::HttpRewrite::Event::attribute(const char *tag, int tag_len,
                                       const char *attr, int attr_len,
                                       const char *value, int val_len,
                                       const char *sep)
{
    std::list<Within>::const_iterator it = m_content->within_list.begin();
    bool subst = false;

    for (; it != m_content->within_list.end(); it++)
    {
        std::string t(tag, tag_len);
        if (it->tag.empty() || regex_match(t, it->tag))
        {
            std::string a(attr, attr_len);
            std::vector<std::string> attr;
            boost::split(attr, it->attr, boost::is_any_of(","));
            size_t i;
            for (i = 0; i < attr.size(); i++)
            {
                if (attr[i].compare("#text") &&
                    yaz_strcasecmp(attr[i].c_str(), a.c_str()) == 0)
                    subst = true;
            }
        }
        if (subst)
            break;
    }

    wrbuf_putc(m_w, ' ');
    wrbuf_write(m_w, attr, attr_len);
    if (value)
    {
        wrbuf_puts(m_w, "=");
        wrbuf_puts(m_w, sep);

        std::string output;
        if (subst)
        {
            std::string s(value, val_len);
            it->exec(m_vars, s, true);
            wrbuf_puts(m_w, s.c_str());
        }
        else
            wrbuf_write(m_w, value, val_len);
        wrbuf_puts(m_w, sep);
    }
}

void yf::HttpRewrite::Event::closeTag(const char *tag, int tag_len)
{
    if (!s_within.empty())
    {
        std::list<Within>::const_iterator it = s_within.top();
        std::string t(tag, tag_len);
        if (regex_match(t, it->tag))
            s_within.pop();
    }
    wrbuf_puts(m_w, "</");
    wrbuf_write(m_w, tag, tag_len);
}

void yf::HttpRewrite::Event::text(const char *value, int len)
{
    std::list<Within>::const_iterator it = m_content->within_list.end();
    if (!s_within.empty())
        it = s_within.top();
    if (it != m_content->within_list.end())
    {
        std::string s(value, len);
        it->exec(m_vars, s, false);
        wrbuf_puts(m_w, s.c_str());
    }
    else
        wrbuf_write(m_w, value, len);
}

static bool embed_quoted_literal(
    std::string &content,
    std::map<std::string, std::string> &vars,
    mp::filter::HttpRewrite::RulePtr ruleptr,
    bool html_context)
{
    bool replace = false;
    std::string res;
    const char *cp = content.c_str();
    const char *cp0 = cp;
    while (*cp)
    {
        if (html_context && !strncmp(cp, "&quot;", 6))
        {
            cp += 6;
            res.append(cp0, cp - cp0);
            cp0 = cp;
            while (*cp)
            {
                if (!strncmp(cp, "&quot;", 6))
                    break;
                if (*cp == '\n')
                    break;
                cp++;
            }
            if (!*cp)
                break;
            std::string s(cp0, cp - cp0);
            if (ruleptr->test_patterns(vars, s, true))
                replace = true;
            cp0 = cp;
            res.append(s);
        }
        else if (*cp == '"' || *cp == '\'')
        {
            int m = *cp;
            cp++;
            res.append(cp0, cp - cp0);
            cp0 = cp;
            while (*cp)
            {
                if (cp[-1] != '\\' && *cp == m)
                    break;
                if (*cp == '\n')
                    break;
                cp++;
            }
            if (!*cp)
                break;
            std::string s(cp0, cp - cp0);
            if (ruleptr->test_patterns(vars, s, true))
                replace = true;
            cp0 = cp;
            res.append(s);
        }
        else if (*cp == '/' && cp[1] == '/')
        {
            while (cp[1] && cp[1] != '\n')
                cp++;
        }
        cp++;
    }
    res.append(cp0, cp - cp0);
    content = res;
    return replace;
}

bool yf::HttpRewrite::Within::exec(
    std::map<std::string, std::string> & vars,
    std::string & txt, bool anchor) const
{
    if (type == "quoted-literal")
    {
        return embed_quoted_literal(txt, vars, rule, true);
    }
    else
    {
        return rule->test_patterns(vars, txt, anchor);
    }
}

bool yf::HttpRewrite::Rule::test_patterns(
    std::map<std::string, std::string> & vars,
    std::string & txt, bool anchor)
{
    bool replaces = false;
    bool first = anchor;
    std::string out;
    std::string::const_iterator start, end;
    start = txt.begin();
    end = txt.end();
    while (1)
    {
        std::list<Replace>::iterator bit = replace_list.end();
        {
            std::string::const_iterator best_pos = txt.end();
            std::list<Replace>::iterator it = replace_list.begin();
            for (; it != replace_list.end(); it++)
            {
                if (it->start_anchor && !first)
                    continue;
                if (regex_search(start, end, it->what, it->re))
                {
                    if (it->what[0].first < best_pos)
                    {
                        best_pos = it->what[0].first;
                        bit = it;
                    }
                }
            }
            if (bit == replace_list.end())
                break;
        }
        first = false;
        replaces = true;
        size_t i;
        for (i = 1; i < bit->what.size(); ++i)
        {
            //check if the group is named
            std::map<int, std::string>::const_iterator git
                = bit->group_index.find(i);
            if (git != bit->group_index.end())
            {   //it is
                vars[git->second] = bit->what[i];
            }

        }
        //prepare replacement string
        std::string rvalue = bit->sub_vars(vars);
        yaz_log(YLOG_LOG, "! Rewritten '%s' to '%s'",
                bit->what.str(0).c_str(), rvalue.c_str());
        out.append(start, bit->what[0].first);
        out.append(rvalue);
        start = bit->what[0].second; //move search forward
    }
    out.append(start, end);
    txt = out;
    return replaces;
}

void yf::HttpRewrite::Replace::parse_groups(std::string pattern)
{
    int gnum = 0;
    bool esc = false;
    const std::string &str = pattern;
    std::string res;
    start_anchor = str[0] == '^';
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
    re = res;
}

std::string yf::HttpRewrite::Replace::sub_vars(
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

yf::HttpRewrite::Phase::Phase() : m_verbose(0)
{
}

void yf::HttpRewrite::Content::parse(
    int verbose,
    std::string &content,
    std::map<std::string, std::string> &vars) const
{
    if (type == "html")
    {
        HTMLParser parser;
        Event ev(this, vars);

        parser.set_verbose(verbose);

        parser.parse(ev, content.c_str());
        content = ev.result();
    }
    if (type == "quoted-literal")
    {
        quoted_literal(content, vars);
    }
}

void yf::HttpRewrite::Content::quoted_literal(
    std::string &content,
    std::map<std::string, std::string> &vars) const
{
    std::list<Within>::const_iterator it = within_list.begin();
    if (it != within_list.end())
        embed_quoted_literal(content, vars, it->rule, false);
}

void yf::HttpRewrite::Content::configure(
    const xmlNode *ptr, std::map<std::string, RulePtr > &rules)
{
    for (; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "within"))
        {
            static const char *names[7] =
                { "header", "attr", "tag", "rule", "reqline", "type", 0 };
            std::string values[6];
            mp::xml::parse_attr(ptr, names, values);
            Within w;
            w.header = values[0];
            w.attr = values[1];
            if (values[2].length() > 0)
                w.tag = values[2];
            std::map<std::string,RulePtr>::const_iterator it =
                rules.find(values[3]);
            if (it == rules.end())
                throw mp::filter::FilterException
                    ("Reference to non-existing rule '" + values[3] +
                     "' in http_rewrite filter");
            w.rule = it->second;
            w.reqline = values[4] == "1";
            w.type = values[5];
            if (w.type.empty() || w.type == "quoted-literal")
                ;
            else
                throw mp::filter::FilterException
                    ("within type must be quoted-literal or none in "
                     " in http_rewrite filter");
            within_list.push_back(w);
        }
    }
}

void yf::HttpRewrite::configure_phase(const xmlNode *ptr, Phase &phase)
{
    static const char *names[2] = { "verbose", 0 };
    std::string values[1];
    values[0] = "0";
    mp::xml::parse_attr(ptr, names, values);

    phase.m_verbose = atoi(values[0].c_str());

    std::map<std::string, RulePtr > rules;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        else if (!strcmp((const char *) ptr->name, "rule"))
        {
            static const char *names[2] = { "name", 0 };
            std::string values[1];
            values[0] = "default";
            mp::xml::parse_attr(ptr, names, values);

            RulePtr rule(new Rule);
            for (xmlNode *p = ptr->children; p; p = p->next)
            {
                if (p->type != XML_ELEMENT_NODE)
                    continue;
                if (!strcmp((const char *) p->name, "rewrite"))
                {
                    Replace replace;
                    std::string from;
                    const struct _xmlAttr *attr;
                    for (attr = p->properties; attr; attr = attr->next)
                    {
                        if (!strcmp((const char *) attr->name,  "from"))
                            from = mp::xml::get_text(attr->children);
                        else if (!strcmp((const char *) attr->name,  "to"))
                            replace.recipe = mp::xml::get_text(attr->children);
                        else
                            throw mp::filter::FilterException
                                ("Bad attribute "
                                 + std::string((const char *) attr->name)
                                 + " in rewrite section of http_rewrite");
                    }
                    yaz_log(YLOG_LOG, "Found rewrite rule from '%s' to '%s'",
                            from.c_str(), replace.recipe.c_str());
                    if (!from.empty())
                    {
                        replace.parse_groups(from);
                        rule->replace_list.push_back(replace);
                    }
                }
                else
                    throw mp::filter::FilterException
                        ("Bad element "
                         + std::string((const char *) p->name)
                         + " in http_rewrite filter");
            }
            rules[values[0]] = rule;
        }
        else if (!strcmp((const char *) ptr->name, "content"))
        {
            static const char *names[3] =
                { "type", "mime", 0 };
            std::string values[2];
            mp::xml::parse_attr(ptr, names, values);
            if (values[0].empty())
            {
                    throw mp::filter::FilterException
                        ("Missing attribute, type for for element "
                         + std::string((const char *) ptr->name)
                         + " in http_rewrite filter");
            }
            Content c;

            c.type = values[0];
            // if (!values[1].empty())
                c.content_re = values[1];
            c.configure(ptr->children, rules);
            phase.content_list.push_back(c);
        }
        else
        {
            throw mp::filter::FilterException
                ("Bad element "
                 + std::string((const char *) ptr->name)
                 + " in http_rewrite filter");
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
            configure_phase(ptr, *req_phase);
        }
        else if (!strcmp((const char *) ptr->name, "response"))
        {
            configure_phase(ptr, *res_phase);
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

