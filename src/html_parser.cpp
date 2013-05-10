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

#define TAG_MAX_LEN 64

#define SPACECHR " \t\r\n\f"

#define DEBUG(x) x

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

namespace mp = metaproxy_1;

mp::HTMLParser::HTMLParser()
{
}

mp::HTMLParser::~HTMLParser()
{
}

static void parse_str(mp::HTMLParserEvent & event, const char * str);

void mp::HTMLParser::parse(mp::HTMLParserEvent & event, const char *str) const
{
    parse_str(event, str);
}

//static C functions follow would probably make sense to wrap this in PIMPL?

static int skipSpace (const char *cp)
{
    int i = 0;
    while (cp[i] && strchr (SPACECHR, cp[i]))
        i++;
    return i;
}

static int skipName (const char *cp, char *dst)
{
    int i;
    int j = 0;
    for (i=0; cp[i] && !strchr (SPACECHR "/>=", cp[i]); i++)
	if (j < TAG_MAX_LEN-1)
	{
	    dst[j] = tolower(cp[j]);
	    j++;
	}
    dst[j] = '\0';
    return i;
}

static int skipAttribute (const char *cp, char *name, char **value)
{
    int i = skipName (cp, name);   
    *value = NULL;
    if (!i)
        return skipSpace (cp);
    i += skipSpace (cp + i);
    if (cp[i] == '=')
    {
        int v0, v1;
        i++;
        i += skipSpace (cp + i);
        if (cp[i] == '\"' || cp[i] == '\'')
        {
            char tr = cp[i];
            v0 = ++i;
            while (cp[i] != tr && cp[i])
                i++; 
            v1 = i;
            if (cp[i])
                i++;
        }
        else
        {
            v0 = i;
            while (cp[i] && !strchr (SPACECHR ">", cp[i]))
                i++;
            v1 = i;
        }
        *value = (char *) malloc (v1 - v0 + 1);
        memcpy (*value, cp + v0, v1-v0);
        (*value)[v1-v0] = '\0';
    }
    i += skipSpace (cp + i);
    return i;
}

static int tagAttrs (mp::HTMLParserEvent & event, 
                     const char *tagName,
                     const char *cp)
{
    int i;
    char attr_name[TAG_MAX_LEN];
    char *attr_value;
    i = skipSpace (cp);
    while (cp[i] && cp[i] != '>')
    {
        int nor = skipAttribute (cp+i, attr_name, &attr_value);
        i += nor;
	if (nor)
	{
	    DEBUG(printf ("------ attr %s=%s\n", attr_name, attr_value));
            event.attribute(tagName, attr_name, attr_value);
	}
        else
        {
            if (!nor)
                i++;
        }
    }
    return i;
}

static int tagStart (mp::HTMLParserEvent & event,
        char *tagName, const char *cp, const char which)
{
    int i = 0;
    i = skipName (cp, tagName);
    switch (which) 
    {
        case '/' : 
            DEBUG(printf ("------ tag close %s\n", tagName));
            event.closeTag(tagName);
            break;
        case '!' : 
            DEBUG(printf ("------ dtd %s\n", tagName)); 
            break;
        case '?' : 
            DEBUG(printf ("------ pi %s\n", tagName)); 
            break;
        default :  
            DEBUG(printf ("------ tag open %s\n", tagName));
            event.openTagStart(tagName);
            break;
    }
    return i;
}

static int tagEnd (mp::HTMLParserEvent & event, const char *tagName, const char *cp)
{
    int i = 0;
    while (cp[i] && cp[i] != '>')
        i++;
    if (cp[i] == '>')
    {
        event.anyTagEnd(tagName);
        i++;
    }
    return i;
}

static char* allocFromRange (const char *start, const char *end)
{
    char *value = (char *) malloc (end - start + 1);
    assert (value);
    memcpy (value, start, end - start);
    value[end - start] = '\0';
    return value;
}

static void tagText (mp::HTMLParserEvent & event, const char *text_start, const char *text_end)
{
    if (text_end - text_start) //got text to flush
    {
        char *temp = allocFromRange(text_start, text_end);
        DEBUG(printf ("------ text %s\n", temp));
        event.text(text_start, text_end-text_start);
        free(temp);
    }
}

static void parse_str (mp::HTMLParserEvent & event, const char *cp)
{
    const char *text_start = cp;
    const char *text_end = cp;
    while (*cp)
    {
        if (cp[0] == '<' && cp[1])  //tag?
        {
            char which = cp[1];
            if (which == '/') cp++;
            if (!strchr (SPACECHR, cp[1])) //valid tag starts
            {
                tagText (event, text_start, text_end); //flush any text
                char tagName[TAG_MAX_LEN];
                cp++;
                if (which == '/')
                {
                    cp += tagStart (event, tagName, cp, which);
                }
                else if (which == '!' || which == '?') //pi or dtd
                {
                    cp++;
                    cp += tagStart (event, tagName, cp, which);
                }
                else
                {
                    cp += tagStart (event, tagName, cp, which);
                    cp += tagAttrs (event, tagName, cp);
                }
                cp += tagEnd (event, tagName, cp);
                text_start = cp;
                text_end = cp;
                continue;
            }
        }
        //text
        cp++;
        text_end = cp;
    }
    tagText (event, text_start, text_end); //flush any text
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

