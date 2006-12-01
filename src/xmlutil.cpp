/* $Id: xmlutil.cpp,v 1.11 2006-12-01 12:37:26 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "xmlutil.hpp"

#include <string.h>


namespace mp = metaproxy_1;
// Doxygen doesn't like mp::xml, so we use this instead
namespace mp_xml = metaproxy_1::xml;

static const std::string metaproxy_ns = "http://indexdata.com/metaproxy";

std::string mp_xml::get_text(const struct _xmlAttr  *ptr)
{
    if (ptr->children->type == XML_TEXT_NODE)
        return std::string((const char *) (ptr->children->content));
    return std::string();
}

std::string mp_xml::get_text(const xmlNode *ptr)
{
    std::string c;
    for (ptr = ptr->children; ptr; ptr = ptr->next)
        if (ptr->type == XML_TEXT_NODE)
            c += std::string((const char *) (ptr->content));
    return c;
}

bool mp_xml::get_bool(const xmlNode *ptr, bool default_value)
{
    if (ptr && ptr->type == XML_TEXT_NODE && ptr->content)
    {
        if (!strcmp((const char *) ptr->content, "true"))
            return true;
        else
            return false;
    }
    return default_value;
}

int mp_xml::get_int(const xmlNode *ptr, int default_value)
{
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

std::string mp_xml::get_route(const xmlNode *node)
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
