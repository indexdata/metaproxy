/* $Id: router_flexml.cpp,v 1.16 2006-01-19 09:41:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include "xmlutil.hpp"
#include "router_flexml.hpp"
#include "factory_filter.hpp"
#include "factory_static.hpp"

#include <iostream>
#include <map>
#include <list>

#include <boost/shared_ptr.hpp>

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


namespace yp2 {
    class RouterFleXML::Route {
        friend class RouterFleXML::Rep;
        friend class RouterFleXML::Pos;
        friend class RouterFleXML;
        std::list<boost::shared_ptr<const yp2::filter::Base> > m_list;
    };
    class RouterFleXML::Rep {
        friend class RouterFleXML;
        friend class RouterFleXML::Pos;
        Rep();

        void base(xmlDocPtr doc, yp2::FactoryFilter &factory);

        typedef std::map<std::string,
                         boost::shared_ptr<const yp2::filter::Base > >
                         IdFilterMap ;

        IdFilterMap m_id_filter_map;

        std::map<std::string,RouterFleXML::Route> m_routes;

        std::string m_start_route;

        std::string m_dl_path;

        void parse_xml_config_dom(xmlDocPtr doc);

        void parse_xml_filters(xmlDocPtr doc, const xmlNode *node);
        void parse_xml_routes(xmlDocPtr doc, const xmlNode *node);

        bool m_xinclude;
    private:
        FactoryFilter *m_factory; // TODO shared_ptr
    };

    class RouterFleXML::Pos : public RoutePos {
    public:
        virtual const filter::Base *move(const char *route);
        virtual RoutePos *clone();
        virtual ~Pos();
        yp2::RouterFleXML::Rep *m_p;

        std::map<std::string, 
                 RouterFleXML::Route>::iterator m_route_it;
        std::list<boost::shared_ptr <const yp2::filter::Base> >::iterator m_filter_it;
    };
}

void yp2::RouterFleXML::Rep::parse_xml_filters(xmlDocPtr doc,
                                               const xmlNode *node)
{
    unsigned int filter_nr = 0;
    while(node && yp2::xml::check_element_yp2(node, "filter"))
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
                throw yp2::XMLError("Only attribute id or type allowed"
                                    " in filter element. Got " + name);
        }

        if (!m_factory->exist(type_value))
        {
            std::cout << "about to load " << type_value << ", path=" << 
                m_dl_path << "\n";
            m_factory->add_creator_dl(type_value, m_dl_path);
        }
        yp2::filter::Base* filter_base = m_factory->create(type_value);

        filter_base->configure(node);

        if (m_id_filter_map.find(id_value) != m_id_filter_map.end())
            throw yp2::XMLError("Filter " + id_value + " already defined");

        m_id_filter_map[id_value] =
            boost::shared_ptr<yp2::filter::Base>(filter_base);

        node = yp2::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
}

void yp2::RouterFleXML::Rep::parse_xml_routes(xmlDocPtr doc,
                                              const xmlNode *node)
{
    yp2::xml::check_element_yp2(node, "route");

    unsigned int route_nr = 0;
    while(yp2::xml::is_element_yp2(node, "route"))
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
                throw yp2::XMLError("Only attribute 'id' allowed for"
                                    " element 'route'."
                                    " Got " + name);
        }

        Route route;

        // process <filter> nodes in third level
        const xmlNode* node3 = yp2::xml::jump_to_children(node, XML_ELEMENT_NODE);

        unsigned int filter3_nr = 0;
        while(node3 && yp2::xml::check_element_yp2(node3, "filter"))
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
                    throw yp2::XMLError("Only attribute 'refid' or 'type'"
                                        " allowed for element 'filter'."
                                        " Got " + name);
            }
            if (refid_value.length())
            {
                std::map<std::string,
                    boost::shared_ptr<const yp2::filter::Base > >::iterator it;
                it = m_id_filter_map.find(refid_value);
                if (it == m_id_filter_map.end())
                    throw yp2::XMLError("Unknown filter refid "
                                        + refid_value);
                else
                    route.m_list.push_back(it->second);
            }
            else if (type_value.length())
            {
                if (!m_factory->exist(type_value))
                {
                    std::cout << "about to load " << type_value << ", path=" << 
                        m_dl_path << "\n";
                    m_factory->add_creator_dl(type_value, m_dl_path);
                }
                yp2::filter::Base* filter_base = m_factory->create(type_value);

                filter_base->configure(node3);
                
                route.m_list.push_back(
                    boost::shared_ptr<yp2::filter::Base>(filter_base));
            }
            node3 = yp2::xml::jump_to_next(node3, XML_ELEMENT_NODE);
            
        }
        std::map<std::string,RouterFleXML::Route>::iterator it;
        it = m_routes.find(id_value);
        if (it != m_routes.end())
            throw yp2::XMLError("Route id='" + id_value
                                + "' already exist");
        else
            m_routes[id_value] = route;
        node = yp2::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
}

void yp2::RouterFleXML::Rep::parse_xml_config_dom(xmlDocPtr doc)
{
    if (!doc)
        throw yp2::XMLError("Empty XML Document");
    
    const xmlNode* root = xmlDocGetRootElement(doc);
    
    yp2::xml::check_element_yp2(root,  "yp2");

    const xmlNode* node = yp2::xml::jump_to_children(root, XML_ELEMENT_NODE);

    if (yp2::xml::is_element_yp2(node, "dlpath"))
    {
        m_dl_path = yp2::xml::get_text(node);
        node = yp2::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
    // process <start> node which is expected first element node
    if (yp2::xml::check_element_yp2(node, "start"))
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
                throw yp2::XMLError("Only attribute start allowed"
                                    " in element 'start'. Got " + name);
        }
        node = yp2::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
    // process <filters> node if given
    if (yp2::xml::is_element_yp2(node, "filters"))
    {
        parse_xml_filters(doc, yp2::xml::jump_to_children(node,
                                                          XML_ELEMENT_NODE));
                      
        node = yp2::xml::jump_to_next(node, XML_ELEMENT_NODE);
    }
    // process <routes> node which is expected third element node
    yp2::xml::check_element_yp2(node, "routes");
    
    parse_xml_routes(doc, yp2::xml::jump_to_children(node, XML_ELEMENT_NODE));

    node = yp2::xml::jump_to_next(node, XML_ELEMENT_NODE);
    if (node)
    {
        throw yp2::XMLError("Unexpected element " 
                            + std::string((const char *)node->name));
    }
}        

yp2::RouterFleXML::Rep::Rep() : m_xinclude(false)
{
}

void yp2::RouterFleXML::Rep::base(xmlDocPtr doc, yp2::FactoryFilter &factory)
{
    m_factory = &factory;
    parse_xml_config_dom(doc);
    m_start_route = "start";
}

yp2::RouterFleXML::RouterFleXML(xmlDocPtr doc, yp2::FactoryFilter &factory)
    : m_p(new Rep)
{
    m_p->base(doc, factory);
}

yp2::RouterFleXML::RouterFleXML(std::string xmlconf, yp2::FactoryFilter &factory) 
    : m_p(new Rep)
{            
    LIBXML_TEST_VERSION;
    
    xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(),
                                   xmlconf.size());
    if (!doc)
        throw yp2::XMLError("xmlParseMemory failed");
    else
    {
        m_p->base(doc, factory);
        xmlFreeDoc(doc);
    }
}

yp2::RouterFleXML::~RouterFleXML()
{
}

const yp2::filter::Base *yp2::RouterFleXML::Pos::move(const char *route)
{
    if (route && *route)
    {
        std::cout << "move to " << route << "\n";
        m_route_it = m_p->m_routes.find(route);
        if (m_route_it == m_p->m_routes.end())
        {
            std::cout << "no such route " << route << "\n";
            throw yp2::XMLError("bad route " + std::string(route));
        }
        m_filter_it = m_route_it->second.m_list.begin();
    }
    if (m_filter_it == m_route_it->second.m_list.end())
        return 0;
    const yp2::filter::Base *f = (*m_filter_it).get();
    m_filter_it++;
    return f;
}

yp2::RoutePos *yp2::RouterFleXML::createpos() const
{
    yp2::RouterFleXML::Pos *p = new yp2::RouterFleXML::Pos;

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

yp2::RoutePos *yp2::RouterFleXML::Pos::clone()
{
    yp2::RouterFleXML::Pos *p = new yp2::RouterFleXML::Pos;
    p->m_filter_it = m_filter_it;
    p->m_route_it = m_route_it;
    p->m_p = m_p;
    return p;
}

yp2::RouterFleXML::Pos::~Pos()
{
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
