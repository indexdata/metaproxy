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

namespace mp = metaproxy_1;
// Doxygen doesn't like mp::xml, so we use this instead
namespace mp_xml = metaproxy_1::xml;

void mp_xml::url_recipe_handle(xmlDoc *doc, std::string recipe)
{
    if (recipe.length() == 0)
        return;
    std::string result;

    size_t p0 = 0, p1 = 0;
    for (;;)
    {
        p1 = recipe.find_first_of("${", p0);
        if (p1 == std::string::npos)
        {
            result += recipe.substr(p0);
            break;
        }
        result += recipe.substr(p0, p1 - p0);

        int step = 0;  // 0=variable, 1=pattern, 2=replacement, 3=mode
        std::string variable;
        std::string pattern;
        std::string replacement;
        std::string mode;
        p0 = p1+2;
        while (p0 < recipe.length() && step < 5)
        {
            char c = recipe[p0];
            if (c == '}')
                step = 5;
            else if (step == 0)
            {
                if (c == '[')
                    step = 1;
                else
                    variable += c;
            }
            else if (step == 1)
            {
                if (c == '/')
                    step = 2;
                else
                    pattern += c;
            }
            else if (step == 2)
            {
                if (c == '/')
                    step = 3;
                else
                    replacement += c;
            }
            else if (step == 3)
            {
                if (c == ']')
                    step = 4;
                else
                    mode += c;
            }
            p0++;
        }
        if (variable.length())
        {
            ;
        }
    }
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

