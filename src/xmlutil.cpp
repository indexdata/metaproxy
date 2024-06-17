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

#include <metaproxy/xmlutil.hpp>

#include <string.h>
#include <stdlib.h>

namespace mp = metaproxy_1;
// Doxygen doesn't like mp::xml, so we use this instead
namespace mp_xml = metaproxy_1::xml;

static const std::string metaproxy_ns = "http://indexdata.com/metaproxy";

std::string mp_xml::get_text(const struct _xmlAttr  *ptr)
{
    return get_text(ptr->children);
}

std::string mp_xml::get_text(const xmlNode *ptr)
{
    std::string c;
    if (ptr && ptr->type != XML_TEXT_NODE)
        ptr = ptr->children;
    for (; ptr; ptr = ptr->next)
        if (ptr->type == XML_TEXT_NODE)
            c += std::string((const char *) (ptr->content));
    return c;
}

bool mp_xml::get_bool(const xmlNode *ptr, bool default_value)
{
    if (ptr && ptr->type != XML_TEXT_NODE)
        ptr = ptr->children;
    if (ptr && ptr->type == XML_TEXT_NODE && ptr->content)
    {
        if (!strcmp((const char *) ptr->content, "true")
            || !strcmp((const char *) ptr->content, "1"))
            return true;
        else
            return false;
    }
    return default_value;
}

int mp_xml::get_int(const xmlNode *ptr, int default_value)
{
    if (ptr && ptr->type != XML_TEXT_NODE)
        ptr = ptr->children;
    if (ptr && ptr->type == XML_TEXT_NODE && ptr->content)
    {
        return atoi((const char *) ptr->content);
    }
    return default_value;
}

bool mp_xml::check_attribute(const _xmlAttr *ptr,
                             const std::string &ns,
                             const std::string &name)
{
    if (!mp::xml::is_attribute(ptr, ns, name))
    {
        std::string got_attr = "'";
        if (ptr && ptr->name)
            got_attr += std::string((const char *)ptr->name);
        if (ns.size() && ptr && ptr->ns && ptr->ns->href){
            got_attr += " ";
            got_attr += std::string((const char *)ptr->ns->href);
         }
        got_attr += "'";

        throw mp::XMLError("Expected XML attribute '" + name
                           + " " + ns + "'"
                           + ", not " + got_attr);
    }
    return true;
}

bool mp_xml::is_attribute(const _xmlAttr *ptr,
                          const std::string &ns,
                          const std::string &name)
{
    if (0 != xmlStrcmp(BAD_CAST name.c_str(), ptr->name))
        return false;

    if (ns.size()
        && (!ptr->ns || !ptr->ns->href
            || 0 != xmlStrcmp(BAD_CAST ns.c_str(), ptr->ns->href)))
        return false;

    return true;
}


bool mp_xml::is_element(const xmlNode *ptr,
                          const std::string &ns,
                          const std::string &name)
{
    if (ptr && ptr->type == XML_ELEMENT_NODE && ptr->ns && ptr->ns->href
        && !xmlStrcmp(BAD_CAST ns.c_str(), ptr->ns->href)
        && !xmlStrcmp(BAD_CAST name.c_str(), ptr->name))
        return true;
    return false;
}

bool mp_xml::is_element_mp(const xmlNode *ptr,
                           const std::string &name)
{
    return mp::xml::is_element(ptr, metaproxy_ns, name);
}


bool mp_xml::check_element_mp(const xmlNode *ptr,
                              const std::string &name)
{
    if (!mp::xml::is_element_mp(ptr, name))
    {
        std::string got_element = "<";
        if (ptr && ptr->name)
            got_element += std::string((const char *)ptr->name);
        if (ptr && ptr->ns && ptr->ns->href){
            got_element += " xmlns=\"";
            got_element += std::string((const char *)ptr->ns->href);
            got_element += "\"";
        }
        got_element += ">";

        throw mp::XMLError("Expected XML element <" + name
                           + " xmlns=\"" + metaproxy_ns + "\">"
                           + ", not " + got_element);
    }
    return true;
}

void mp_xml::parse_attr(const xmlNode *node, const char **names,
                        std::string *values)
{
    size_t i;
    for (i = 0; names[i]; i++)
        values[i].clear();

    if (node)
    {
        const struct _xmlAttr *attr;
        for (attr = node->properties; attr; attr = attr->next)
        {
            std::string value;
            const char *name = (const char *) attr->name;

            if (attr->children && attr->children->type == XML_TEXT_NODE)
                value = std::string((const char *)attr->children->content);
            for (i = 0; names[i]; i++)
                if (!strcmp(name, names[i]))
                {
                    values[i] = value;
                    break;
                }
            if (!names[i])
            {
                throw XMLError("Unsupported attribute: '" +
                               std::string(name) +
                               "' in element '" + 
                               std::string((const char *) node->name) + "'");
            }
        }
    }
}

std::string mp_xml::get_route(const xmlNode *node, std::string &auth)
{
    const char *names[3] = { "route", "auth", 0 };
    std::string values[2];

    parse_attr(node, names, values);

    auth = values[1];
    return values[0];
}

std::string mp_xml::get_route(const xmlNode *node)
{
    const char *names[2] = { "route", 0 };
    std::string values[1];

    parse_attr(node, names, values);

    return values[0];
}

const xmlNode* mp_xml::jump_to_children(const xmlNode* node,
                                          int xml_node_type)
{
    node = node->children;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

const xmlNode* mp_xml::jump_to_next(const xmlNode* node,
                                      int xml_node_type)
{
    node = node->next;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

const xmlNode* mp_xml::jump_to(const xmlNode* node,
                                 int xml_node_type)
{
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

void mp_xml::check_empty(const xmlNode *node)
{
    if (node)
    {
        const xmlNode *n;
        const struct _xmlAttr *attr;
        std::string extra;
        for (attr = node->properties; attr; attr = attr->next)
            if (!strcmp((const char *) attr->name, "type"))
                extra = " of type " + get_text(attr);
        for (n = node->children; n; n = n->next)
            if (n->type == XML_ELEMENT_NODE)
                throw mp::XMLError("No child elements allowed inside element "
                                   + std::string((const char *) node->name)
                                   + extra);
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

