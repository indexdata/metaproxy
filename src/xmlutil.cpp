/* $Id: xmlutil.cpp,v 1.5 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "xmlutil.hpp"

namespace mp = metaproxy_1;

std::string mp::xml::get_text(const xmlNode *ptr)
{
    std::string c;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
        if (ptr->type == XML_TEXT_NODE)
            c += std::string((const char *) (ptr->content));
    return c;
}


bool mp::xml::is_element(const xmlNode *ptr, 
                          const std::string &ns,
                          const std::string &name)
{
    if (ptr && ptr->type == XML_ELEMENT_NODE && ptr->ns && ptr->ns->href 
        && !xmlStrcmp(BAD_CAST ns.c_str(), ptr->ns->href)
        && !xmlStrcmp(BAD_CAST name.c_str(), ptr->name))
        return true;
    return false;
}

bool mp::xml::is_element_yp2(const xmlNode *ptr, 
                              const std::string &name)
{
    return mp::xml::is_element(ptr, "http://indexdata.dk/yp2/config/1", name);
}


bool mp::xml::check_element_yp2(const xmlNode *ptr, 
                                 const std::string &name)
{
    if (!mp::xml::is_element_yp2(ptr, name))
        throw mp::XMLError("Expected element name " + name);
    return true;
}

std::string mp::xml::get_route(const xmlNode *node)
{
    std::string route_value;
    if (node)
    {
        const struct _xmlAttr *attr;
        for (attr = node->properties; attr; attr = attr->next)
        {
            std::string name = std::string((const char *) attr->name);
            std::string value;
            
            if (attr->children && attr->children->type == XML_TEXT_NODE)
                value = std::string((const char *)attr->children->content);
            
            if (name == "route")
                route_value = value;
            else
                throw XMLError("Only attribute route allowed"
                               " in " + std::string((const char *)node->name)
                               + " element. Got " + std::string(name));
        }
    }
    return route_value;
}


const xmlNode* mp::xml::jump_to_children(const xmlNode* node,
                                          int xml_node_type)
{
    node = node->children;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

const xmlNode* mp::xml::jump_to_next(const xmlNode* node,
                                      int xml_node_type)
{
    node = node->next;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

const xmlNode* mp::xml::jump_to(const xmlNode* node,
                                 int xml_node_type)
{
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

void mp::xml::check_empty(const xmlNode *node)
{
    if (node)
    {
        const xmlNode *n;
        for (n = node->children; n; n = n->next)
            if (n->type == XML_ELEMENT_NODE)
                throw mp::XMLError("No child elements allowed inside element "
                                    + std::string((const char *) node->name));
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
