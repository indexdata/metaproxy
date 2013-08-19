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
#include "html_parser.hpp"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#define SPACECHR " \t\r\n\f"


namespace metaproxy_1 {
    class HTMLParser::Rep {
        friend class HTMLParser;
    public:
        void parse_str(HTMLParserEvent &event, const char *cp);
        void tagText(HTMLParserEvent &event,
                     const char *text_start, const char *text_end);
        int tagEnd(HTMLParserEvent &event,
                   const char *tag, int tag_len, const char *cp);
        int tagAttrs(HTMLParserEvent &event,
                     const char *name, int len,
                     const char *cp);
        int skipAttribute(HTMLParserEvent &event,
                          const char *cp, int *attr_len,
                          const char **value, int *val_len, int *tr);
        Rep();
        ~Rep();
        int m_verbose;
    };
}

namespace mp = metaproxy_1;

mp::HTMLParser::Rep::Rep()
{
    m_verbose = 0;
}

mp::HTMLParser::Rep::~Rep()
{
}

mp::HTMLParser::HTMLParser() : m_p(new Rep)
{
}

mp::HTMLParser::~HTMLParser()
{
}

void mp::HTMLParser::set_verbose(int v)
{
    m_p->m_verbose = v;
}


void mp::HTMLParser::parse(mp::HTMLParserEvent & event, const char *str) const
{
    m_p->parse_str(event, str);
}

static int isAlpha(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int skipSpace(const char *cp)
{
    int i = 0;
    while (cp[i] && strchr(SPACECHR, cp[i]))
        i++;
    return i;
}

static int skipName(const char *cp)
{
    int i;
    for (i = 0; cp[i] && !strchr(SPACECHR "/>=<", cp[i]); i++)
        ;
    return i;
}

int mp::HTMLParser::Rep::skipAttribute(HTMLParserEvent &event,
                                       const char *cp, int *attr_len,
                                       const char **value, int *val_len,
                                       int *tr)
{
    int v0, v1;
    int i = skipName(cp);
    *attr_len = i;
    *value = NULL;
    if (!i)
        return skipSpace(cp);
    i += skipSpace(cp + i);
    if (cp[i] == '=')
    {
        i++;
        i += skipSpace(cp + i);
        if (cp[i] == '\"' || cp[i] == '\'')
        {
            *tr = cp[i];
            v0 = ++i;
            while (cp[i] != *tr && cp[i])
                i++;
            v1 = i;
            if (cp[i])
                i++;
        }
        else
        {
            *tr = 0;
            v0 = i;
            while (cp[i] && !strchr(SPACECHR ">", cp[i]))
                i++;
            v1 = i;
        }
        *value = cp + v0;
        *val_len = v1 - v0;
        i += skipSpace(cp + i);
    }
    return i;
}

int mp::HTMLParser::Rep::tagAttrs(HTMLParserEvent &event,
                                  const char *name, int len,
                                  const char *cp)
{
    int i = skipSpace(cp);
    while (cp[i] && cp[i] != '>' && cp[i] != '/' && cp[i] != '<')
    {
        const char *attr_name = cp + i;
        int attr_len;
        const char *value;
        int val_len;
        int tr;
        char x[2];
        int nor = skipAttribute(event, cp+i, &attr_len, &value, &val_len, &tr);
        if (!nor)
            break;
        i += nor;

        x[0] = tr;
        x[1] = 0;
        if (m_verbose)
            printf ("------ attr %.*s=%.*s\n", attr_len, attr_name,
                    val_len, value);
        event.attribute(name, len, attr_name, attr_len, value, val_len, x);
    }
    return i;
}

int mp::HTMLParser::Rep::tagEnd(HTMLParserEvent &event,
                                const char *tag, int tag_len, const char *cp)
{
    int i = 0;
    int close_it = 0;
    for (; cp[i] && cp[i] != '/' && cp[i] != '>' && cp[i] != '<'; i++)
        ;
    if (i > 0)
    {
        if (m_verbose)
            printf("------ text %.*s\n", i, cp);
        event.text(cp, i);
    }
    if (cp[i] == '/')
    {
        close_it = 1;
        i++;
    }
    if (cp[i] == '>')
    {
        if (m_verbose)
            printf("------ any tag %s %.*s\n",
                   close_it ? " close" : "end", tag_len, tag);
        event.anyTagEnd(tag, tag_len, close_it);
        i++;
    }
    return i;
}

void mp::HTMLParser::Rep::tagText(HTMLParserEvent &event,
                                  const char *text_start, const char *text_end)
{
    if (text_end - text_start) //got text to flush
    {
        if (m_verbose)
            printf("------ text %.*s\n",
                   (int) (text_end - text_start), text_start);
        event.text(text_start, text_end-text_start);
    }
}

void mp::HTMLParser::Rep::parse_str(HTMLParserEvent &event, const char *cp)
{
    const char *text_start = cp;
    while (*cp)
    {
        if (*cp++ != '<')
            continue;

        if (*cp == '!')
        {
            int i;
            tagText(event, text_start, cp - 1);
            if (cp[1] == '-' && cp[2] == '-')
            {
                for (i = 3; cp[i]; i++)
                    if (cp[i] == '-' && cp[i+1] == '-' && cp[i+2] == '>')
                    {
                        i+= 2;
                        event.openTagStart(cp, i);
                        break;
                    }
            }
            else
            {
                for (i = 1; cp[i] && cp[i] != '>'; i++)
                    ;
                event.openTagStart(cp, i);
            }
            if (m_verbose)
                printf("------ dtd %.*s\n", i, cp);
            i += tagEnd(event, cp, i, cp + i);
            cp += i;
            text_start = cp;
        }
        else if (*cp == '?')
        {
            int i;
            tagText(event, text_start, cp - 1);
            for (i = 1; cp[i] && cp[i] != '>'; i++)
                ;
            event.openTagStart(cp, i);
            if (m_verbose)
                printf("------ pi %.*s\n", i, cp);
            i += tagEnd(event, cp, i, cp + i);
            cp += i;
            text_start = cp;
        }
        else if (*cp == '/' && isAlpha(cp[1]))
        {
            int i;
            tagText(event, text_start, cp - 1);

            i = skipName(++cp);
            event.closeTag(cp, i);
            if (m_verbose)
                printf("------ tag close %.*s\n", i, cp);
            i += tagEnd(event, cp, i, cp + i);
            cp += i;
            text_start = cp;
        }
        else if (isAlpha(*cp))
        {
            int i, j;
            tagText(event, text_start, cp - 1);
            i = skipName(cp);
            event.openTagStart(cp, i);
            if (m_verbose)
                printf("------ tag open %.*s\n", i, cp);
            j = tagAttrs(event, cp, i, cp + i);
            j += tagEnd(event, cp, i, cp + i + j);
            cp += i + j;
            text_start = cp;
        }
    }
    tagText(event, text_start, cp);
}

mp::HTMLParserEvent::~HTMLParserEvent()
{
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

