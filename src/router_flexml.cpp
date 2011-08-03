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
#include <metaproxy/xmlutil.hpp>
#include "router_flexml.hpp"
#include "factory_filter.hpp"
#include "factory_static.hpp"

#include <iostream>
#include <map>
#include <list>
#include <yaz/log.h>
#include <yaz/xml_include.h>

#include <boost/shared_ptr.hpp>

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    class RouterFleXML::Route {
        friend class RouterFleXML::Rep;
        friend class RouterFleXML::Pos;
        friend class RouterFleXML;
        std::list<boost::shared_ptr<const mp::filter::Base> > m_list;
    };
    class RouterFleXML::Rep {
        friend class RouterFleXML;
        friend class RouterFleXML::Pos;
        Rep();

        void base(xmlDocPtr doc, mp::FactoryFilter &factory, bool test_only,
                  const char *file_include_path);

        typedef std::map<std::string,
                         boost::shared_ptr<const mp::filter::Base > >
                         IdFilterMap ;

        IdFilterMap m_id_filter_map;

        std::map<std::string,RouterFleXML::Route> m_routes;

        std::string m_start_route;

        std::string m_dl_path;

        void parse_xml_config_dom(xmlDocPtr doc, bool test_only,
                                  const char *file_include_path);

        void parse_xml_filters(xmlDocPtr doc, const xmlNode *node,
                               bool test_only, const char *file_include_path);
        void parse_xml_routes(xmlDocPtr doc, const xmlNode *node,
                              bool test_only, const char *file_include_path);

        bool m_xinclude;
    private:
        FactoryFilter *m_factory; // TODO shared_ptr
    };

    class RouterFleXML::Pos : public RoutePos {
    public:
        virtual const filter::Base *move(const char *route);
        virtual RoutePos *clone();
        virtual ~Pos();
        mp::RouterFleXML::Rep *m_p;

        std::map<std::string, 
                 RouterFleXML::Route>::iterator m_route_it;
        std::list<boost::shared_ptr <const mp::filter::Base> >::iterator m_filter_it;
    };
}

void mp::RouterFleXML::Rep::parse_xml_filters(xmlDocPtr doc,
                                              const xmlNode *node,
                                              bool test_only,
                                              const char *file_include_path)
{
    unsigned int filter_nr = 0;
    while(node && mp::xml::check_element_mp(node, "filter"))
    {
        filter_nr++;

        const struct _xmlAttr *attr;
        std::string id_value;
        std::string type_value;
        for (attr = node->properties; attr; attr = attr->next)
        {
            std::string name = std::string((const char *) attr->name);
            std::string value;

            if (attr->children && attr->children->type == XML_TEXT_NODE)
                value = std::string((const char *)attr->children->content);

            if (name == "id")
                id_value = value;
            else if (name == "type")
                type_value = value;
            else
                throw mp::XMLError("Only attribute id or type allowed"
                                    " in filter element. Got " + name);
        }

        if (!m_factory->exist(type_value))
        {
            yaz_log(YLOG_LOG, "Loading %s (dlpath %s)",
                    type_value.c_str(), m_dl_path.c_str());
            m_factory->add_creator_dl(type_value, m_dl_path);
        }
        mp::filter::Base* filter_base = m_factory->create(type_value);

        filter_base->configure(node, test_only, file_include_path);

        if (m_id_filter_map.find(id_value) != m_id_filter_map.end())
            throw mp::XMLError("Filter " + id_value + " already defined");

        m_id_filter_map[id_value] =
            boost::shared_ptr<mp::filter::Base>(filter_base);

        node = mp::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
}

void mp::RouterFleXML::Rep::parse_xml_routes(xmlDocPtr doc,
                                             const xmlNode *node,
                                             bool test_only,
                                             const char *file_include_path)
{
    mp::xml::check_element_mp(node, "route");

    unsigned int route_nr = 0;
    while(mp::xml::is_element_mp(node, "route"))
    {
        route_nr++;

        const struct _xmlAttr *attr;
        std::string id_value;
        for (attr = node->properties; attr; attr = attr->next)
        {
            std::string name = std::string((const char *) attr->name);
            std::string value;
            
            if (attr->children && attr->children->type == XML_TEXT_NODE)
                value = std::string((const char *)attr->children->content);
            
            if (name == "id")
                id_value = value;
            else
                throw mp::XMLError("Only attribute 'id' allowed for"
                                    " element 'route'."
                                    " Got " + name);
        }

        Route route;

        // process <filter> nodes in third level
        const xmlNode* node3 = mp::xml::jump_to_children(node, XML_ELEMENT_NODE);

        unsigned int filter3_nr = 0;
        while(node3 && mp::xml::check_element_mp(node3, "filter"))
        {
            filter3_nr++;
            
            const struct _xmlAttr *attr;
            std::string refid_value;
            std::string type_value;
            for (attr = node3->properties; attr; attr = attr->next)
            {
                std::string name = std::string((const char *) attr->name);
                std::string value;
                
                if (attr->children && attr->children->type == XML_TEXT_NODE)
                    value = std::string((const char *)attr->children->content);
                
                if (name == "refid")
                    refid_value = value;
                else if (name == "type")
                    type_value = value;
                else
                    throw mp::XMLError("Only attribute 'refid' or 'type'"
                                        " allowed for element 'filter'."
                                        " Got " + name);
            }
            if (refid_value.length())
            {
                std::map<std::string,
                    boost::shared_ptr<const mp::filter::Base > >::iterator it;
                it = m_id_filter_map.find(refid_value);
                if (it == m_id_filter_map.end())
                    throw mp::XMLError("Unknown filter refid "
                                        + refid_value);
                else
                    route.m_list.push_back(it->second);
            }
            else if (type_value.length())
            {
                if (!m_factory->exist(type_value))
                {
                    yaz_log(YLOG_LOG, "Loading %s (dlpath %s)",
                            type_value.c_str(), m_dl_path.c_str());
                    m_factory->add_creator_dl(type_value, m_dl_path);
                }
                mp::filter::Base* filter_base = m_factory->create(type_value);

                filter_base->configure(node3, test_only, file_include_path);
                
                route.m_list.push_back(
                    boost::shared_ptr<mp::filter::Base>(filter_base));
            }
            node3 = mp::xml::jump_to_next(node3, XML_ELEMENT_NODE);
            
        }
        std::map<std::string,RouterFleXML::Route>::iterator it;
        it = m_routes.find(id_value);
        if (it != m_routes.end())
            throw mp::XMLError("Route id='" + id_value
                                + "' already exist");
        else
            m_routes[id_value] = route;
        node = mp::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
}

void mp::RouterFleXML::Rep::parse_xml_config_dom(xmlDocPtr doc,
                                                 bool test_only,
                                                 const char *file_include_path)
{
    if (!doc)
        throw mp::XMLError("Empty XML Document");
    
    const xmlNode* root = xmlDocGetRootElement(doc);

    if (file_include_path)
    {
        int r = yaz_xml_include_simple((xmlNode *) root, file_include_path);
        if (r)
            throw mp::XMLError("YAZ XML Include failed");
    }
    
    mp::xml::check_element_mp(root,  "metaproxy");

    const xmlNode* node = mp::xml::jump_to_children(root, XML_ELEMENT_NODE);

    if (mp::xml::is_element_mp(node, "dlpath"))
    {
        m_dl_path = mp::xml::get_text(node);
        node = mp::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
    // process <start> node which is expected first element node
    if (mp::xml::check_element_mp(node, "start"))
    {
        const struct _xmlAttr *attr;
        std::string id_value;
        for (attr = node->properties; attr; attr = attr->next)
        {
            std::string name = std::string((const char *) attr->name);
            std::string value;

            if (attr->children && attr->children->type == XML_TEXT_NODE)
                value = std::string((const char *)attr->children->content);

            if (name == "route")
                m_start_route = value;
            else
                throw mp::XMLError("Only attribute start allowed"
                                    " in element 'start'. Got " + name);
        }
        node = mp::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
    // process <filters> node if given
    if (mp::xml::is_element_mp(node, "filters"))
    {
        parse_xml_filters(doc, mp::xml::jump_to_children(node,
                                                         XML_ELEMENT_NODE),
                          test_only, file_include_path);
                      
        node = mp::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
    // process <routes> node which is expected third element node
    mp::xml::check_element_mp(node, "routes");
    
    parse_xml_routes(doc, mp::xml::jump_to_children(node, XML_ELEMENT_NODE),
                     test_only, file_include_path);

    node = mp::xml::jump_to_next(node, XML_ELEMENT_NODE);
    if (node)
    {
        throw mp::XMLError("Unexpected element " 
                            + std::string((const char *)node->name));
    }
}        

mp::RouterFleXML::Rep::Rep() : m_xinclude(false)
{
}

void mp::RouterFleXML::Rep::base(xmlDocPtr doc, mp::FactoryFilter &factory,
                                 bool test_only, const char *file_include_path)
{
    m_factory = &factory;
    parse_xml_config_dom(doc, test_only, file_include_path);
    m_start_route = "start";
}

mp::RouterFleXML::RouterFleXML(xmlDocPtr doc, mp::FactoryFilter &factory,
                               bool test_only, const char *file_include_path)
    : m_p(new Rep)
{
    m_p->base(doc, factory, test_only, file_include_path);
}

mp::RouterFleXML::RouterFleXML(std::string xmlconf, mp::FactoryFilter &factory,
    bool test_only) 
    : m_p(new Rep)
{            
    LIBXML_TEST_VERSION;
    
    xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(),
                                   xmlconf.size());
    if (!doc)
        throw mp::XMLError("xmlParseMemory failed");
    else
    {
        m_p->base(doc, factory, test_only, 0);
        xmlFreeDoc(doc);
    }
}

mp::RouterFleXML::~RouterFleXML()
{
}

const mp::filter::Base *mp::RouterFleXML::Pos::move(const char *route)
{
    if (route && *route)
    {
        //std::cout << "move to " << route << "\n";
        m_route_it = m_p->m_routes.find(route);
        if (m_route_it == m_p->m_routes.end())
        {
            std::cout << "no such route " << route << "\n";
            throw mp::XMLError("bad route " + std::string(route));
        }
        m_filter_it = m_route_it->second.m_list.begin();
    }
    if (m_filter_it == m_route_it->second.m_list.end())
        return 0;
    const mp::filter::Base *f = (*m_filter_it).get();
    m_filter_it++;
    return f;
}

mp::RoutePos *mp::RouterFleXML::createpos() const
{
    mp::RouterFleXML::Pos *p = new mp::RouterFleXML::Pos;

    p->m_route_it = m_p->m_routes.find(m_p->m_start_route);
    if (p->m_route_it == m_p->m_routes.end())
    {
        delete p;
        return 0;
    }
    p->m_filter_it = p->m_route_it->second.m_list.begin();
    p->m_p = m_p.get();
    return p;
}

mp::RoutePos *mp::RouterFleXML::Pos::clone()
{
    mp::RouterFleXML::Pos *p = new mp::RouterFleXML::Pos;
    p->m_filter_it = m_filter_it;
    p->m_route_it = m_route_it;
    p->m_p = m_p;
    return p;
}

mp::RouterFleXML::Pos::~Pos()
{
}


void mp::RouterFleXML::start()
{
    std::map<std::string,RouterFleXML::Route>::iterator route_it;

    route_it = m_p->m_routes.begin();
    while (route_it != m_p->m_routes.end())
    {
        RouterFleXML::Route route = route_it->second;

        std::list<boost::shared_ptr<const mp::filter::Base> >::iterator it;

        for (it = route.m_list.begin(); it != route.m_list.end(); it++)
            (*it)->start();
        route_it++;
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

