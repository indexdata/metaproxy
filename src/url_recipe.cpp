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

#include "config.hpp"

#include <boost/regex.hpp>
#include <metaproxy/xmlutil.hpp>

#include <string.h>

namespace mp_xml = metaproxy_1::xml;

std::string mp_xml::url_recipe_handle(xmlDoc *doc, std::string recipe)
{
    std::string result;
    if (recipe.length() == 0)
        return result;

    const xmlNode *ptr1 = xmlDocGetRootElement(doc);
    while (ptr1 && ptr1->type != XML_ELEMENT_NODE)
        ptr1 = ptr1->next;
    if (ptr1)
        ptr1 = ptr1->children;

    size_t p0 = 0;
    for (;;)
    {
        size_t p1 = recipe.find_first_of("${", p0);
        if (p1 == std::string::npos)
        {
            result += recipe.substr(p0);
            break;
        }
        result += recipe.substr(p0, p1 - p0);
        p0 = p1+2;

        int step = 0;  // 0=variable, 1=pattern, 2=replacement, 3=mode
        std::string variable;
        std::string pattern;
        std::string replacement;
        std::string mode;
        int c_prev = 0;
        while (p0 < recipe.length() && step < 5)
        {
            char c = recipe[p0];
            int c_check = c;
            if (c_prev == '\\')
                c_check = 0;
            
            if (c_check == '}')
                step = 5;
            else if (step == 0)
            {
                if (c_check == '[')
                    step = 1;
                else
                    variable += c;
            }
            else if (c_check == ']')
                step = 4;
            else if (step == 1)
            {
                if (c_check == '/')
                    step = 2;
                else
                    pattern += c;
            }
            else if (step == 2)
            {
                if (c_check == '/')
                    step = 3;
                else
                    replacement += c;
            }
            else if (step == 3)
            {
                mode += c;
            }
            c_prev = c;
            p0++;
        }
        if (variable.length())
        {
            std::string text;
            size_t offset = 0;
            size_t md_pos = variable.find_first_of("md-");
            if (md_pos == 0)
                offset = 3;
            const xmlNode *ptr = ptr1;
            for (; ptr; ptr = ptr->next)
                if (ptr->type == XML_ELEMENT_NODE
                    && !strcmp((const char *) ptr->name, "metadata"))
                {
                    const _xmlAttr *attr = ptr->properties;
                    for (; attr; attr = attr->next)
                        if (!strcmp((const char *) attr->name, "type")
                            && attr->children
                            && !strcmp((const char *) attr->children->content,
                                       variable.c_str() + offset))
                        {
                            text = mp_xml::get_text(ptr);
                            break;
                        }
                }
            boost::regex::flag_type b_mode = boost::regex::perl;
            if (mode.find_first_of('i') != std::string::npos)
                b_mode |= boost::regex::icase;
            boost::regex e(pattern, b_mode);

            boost::match_flag_type match_mode = boost::format_first_only;
            if (mode.find_first_of('g') != std::string::npos)
                match_mode = boost::format_all;
            result += regex_replace(text, e, replacement, match_mode);
        }
    }
    return result;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

