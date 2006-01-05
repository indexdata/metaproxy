/* $Id: router_flexml.cpp,v 1.10 2006-01-05 16:39:37 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
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
        std::list<boost::shared_ptr<const yp2::filter::Base> > m_list;
    };
    class RouterFleXML::Rep {
        friend class RouterFleXML;
        Rep();

        typedef std::map<std::string,
                         boost::shared_ptr<const yp2::filter::Base > >
                         IdFilterMap ;

        IdFilterMap m_id_filter_map;

        std::map<std::string,RouterFleXML::Route> m_routes;

        void parse_xml_config_dom(xmlDocPtr doc);

        bool is_element(const xmlNode *ptr, 
                        const std::string &ns,
                        const std::string &name);
        
        bool is_element_yp2(const xmlNode *ptr, 
                            const std::string &name);

        bool check_element_yp2(const xmlNode *ptr, 
                               const std::string &name);
        
        const xmlNode* jump_to(const xmlNode* node, int xml_node_type);

        const xmlNode* jump_to_next(const xmlNode* node, int xml_node_type);
        
        const xmlNode* jump_to_children(const xmlNode* node, int xml_node_type);
        void parse_xml_filters(xmlDocPtr doc, const xmlNode *node);
        void parse_xml_routes(xmlDocPtr doc, const xmlNode *node);

        bool m_xinclude;
    private:
        FactoryFilter *m_factory; // TODO shared_ptr
    };
}

const xmlNode* yp2::RouterFleXML::Rep::jump_to_children(const xmlNode* node,
                                                        int xml_node_type)
{
    node = node->children;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}
        
const xmlNode* yp2::RouterFleXML::Rep::jump_to_next(const xmlNode* node,
                                                    int xml_node_type)
{
    node = node->next;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}
        
const xmlNode* yp2::RouterFleXML::Rep::jump_to(const xmlNode* node,
                                               int xml_node_type)
{
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

bool yp2::RouterFleXML::Rep::is_element(const xmlNode *ptr, 
                                        const std::string &ns,
                                        const std::string &name)
{
    if (ptr && ptr->type == XML_ELEMENT_NODE && ptr->ns && ptr->ns->href 
        && !xmlStrcmp(BAD_CAST ns.c_str(), ptr->ns->href)
        && !xmlStrcmp(BAD_CAST name.c_str(), ptr->name))
        return true;
    return false;
}

bool yp2::RouterFleXML::Rep::is_element_yp2(const xmlNode *ptr, 
                                            const std::string &name)
{
    return is_element(ptr, "http://indexdata.dk/yp2/config/1", name);
}

bool yp2::RouterFleXML::Rep::check_element_yp2(const xmlNode *ptr, 
                                               const std::string &name)
{
    if (!is_element_yp2(ptr, name))
        throw XMLError("Expected element name " + name);
    return true;
}


void yp2::RouterFleXML::Rep::parse_xml_filters(xmlDocPtr doc,
                                               const xmlNode *node)
{
    unsigned int filter_nr = 0;
    while(node && check_element_yp2(node, "filter"))
    {
        filter_nr++;
        std::cout << "processing /yp2/filters/filter[" 
                  << filter_nr << "]" << std::endl;

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
                throw XMLError("Only attribute id or type allowed"
                               " in filter element. Got " + name);
                
            std::cout << "attr " << name << "=" << value << "\n";
        }

        yp2::filter::Base* filter_base = m_factory->create(type_value);

        filter_base->configure(node);

        if (m_id_filter_map.find(id_value) != m_id_filter_map.end())
            throw XMLError("Filter " + id_value + " already defined");

        m_id_filter_map[id_value] =
            boost::shared_ptr<yp2::filter::Base>(filter_base);

        node = jump_to_next(node, XML_ELEMENT_NODE);
    }
}

void yp2::RouterFleXML::Rep::parse_xml_routes(xmlDocPtr doc,
                                              const xmlNode *node)
{
    check_element_yp2(node, "route");

    unsigned int route_nr = 0;
    while(is_element_yp2(node, "route"))
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
                throw XMLError("Only attribute refid allowed route element. Got " + name);
            
            std::cout << "attr " << name << "=" << value << "\n";
        }

        Route route;

        std::cout << "processing /yp2/routes/route[" 
                  << route_nr << "]" << std::endl;
        
        // process <filter> nodes in third level

        const xmlNode* node3 = jump_to_children(node, XML_ELEMENT_NODE);

        unsigned int filter3_nr = 0;
        while(node3 && check_element_yp2(node3, "filter"))
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
                    throw XMLError("Only attribute refid or type"
                                   " allowed in filter element. Got " + name);
                
                std::cout << "attr " << name << "=" << value << "\n";
            }
            if (refid_value.length())
            {
                std::map<std::string,
                    boost::shared_ptr<const yp2::filter::Base > >::iterator it;
                it = m_id_filter_map.find(refid_value);
                if (it == m_id_filter_map.end())
                    throw XMLError("Unknown filter refid " + refid_value);
                else
                    route.m_list.push_back(it->second);
            }
            else if (type_value.length())
            {
                yp2::filter::Base* filter_base = m_factory->create(type_value);

                filter_base->configure(node3);
                
                route.m_list.push_back(
                    boost::shared_ptr<yp2::filter::Base>(filter_base));
            }
            std::cout << "processing /yp2/routes/route[" 
                      << route_nr << "]/filter[" 
                      << filter3_nr << "]" << std::endl;
            
            node3 = jump_to_next(node3, XML_ELEMENT_NODE);
            
        }
        std::map<std::string,RouterFleXML::Route>::iterator it;
        it = m_routes.find(id_value);
        if (it != m_routes.end())
            throw XMLError("Route id='" + id_value + "' already exist");
        else
            m_routes[id_value] = route;
        node = jump_to_next(node, XML_ELEMENT_NODE);
    }
}

void yp2::RouterFleXML::Rep::parse_xml_config_dom(xmlDocPtr doc)
{
    if (!doc)
        throw XMLError("Empty XML Document");
    
    const xmlNode* root = xmlDocGetRootElement(doc);
    
    check_element_yp2(root,  "yp2");

    std::cout << "processing /yp2" << std::endl;
    
    // process <start> node which is expected first element node
    const xmlNode* node = jump_to_children(root, XML_ELEMENT_NODE);
    //for (; node && node->type != XML_ELEMENT_NODE; node = node->next)
    //    ;

    check_element_yp2(node, "start");

    std::cout << "processing /yp2/start" << std::endl;
    
    // process <filters> node which is expected second element node
    node = jump_to_next(node, XML_ELEMENT_NODE);
    check_element_yp2(node, "filters");
    std::cout << "processing /yp2/filters" << std::endl;
    
    parse_xml_filters(doc, jump_to_children(node, XML_ELEMENT_NODE));
                      
    // process <routes> node which is expected third element node
    node = jump_to_next(node, XML_ELEMENT_NODE);
    check_element_yp2(node, "routes");
    std::cout << "processing /yp2/routes" << std::endl;
    
    parse_xml_routes(doc, jump_to_children(node, XML_ELEMENT_NODE));
}        

yp2::RouterFleXML::Rep::Rep() : m_xinclude(false)
{
}

yp2::RouterFleXML::RouterFleXML(std::string xmlconf, yp2::FactoryFilter &factory) 
    : m_p(new Rep)
{            

    m_p->m_factory = &factory;

    LIBXML_TEST_VERSION;
    
    xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(),
                                   xmlconf.size());
    if (!doc)
        throw XMLError("xmlParseMemory failed");
    else
    {
        m_p->parse_xml_config_dom(doc);
        xmlFreeDoc(doc);
    }
}

yp2::RouterFleXML::~RouterFleXML()
{
}

const yp2::filter::Base *
yp2::RouterFleXML::move(const yp2::filter::Base *filter,
                        const yp2::Package *package) const 
{
    return 0;
}
        

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
