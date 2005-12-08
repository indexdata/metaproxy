/* $Id: router_flexml.cpp,v 1.4 2005-12-08 15:10:34 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include "router_flexml.hpp"

#include <iostream>
#include <map>
#include <list>

#include <boost/shared_ptr.hpp>

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace yp2 {
    class RouterFleXML::Rep {
        friend class RouterFleXML;
        Rep();

        typedef std::map<std::string, boost::shared_ptr<const yp2::filter::Base> >
                IdFilterMap ;
        typedef std::list<std::string> FilterIdList;
        typedef std::map<std::string, FilterIdList > IdRouteMap ;

        std::string m_xmlconf;
        bool m_xinclude;
        xmlDoc * m_xmlconf_doc;
        IdFilterMap m_id_filter_map;
        FilterIdList m_filter_id_list;
        IdRouteMap m_id_route_map;
        void xml_dom_error (const xmlNode* node, std::string msg);

        void create_filter(std::string type, 
                           const xmlDoc * xmldoc,
                           std::string id = "");

        void parse_xml_config_dom();
        
        const xmlNode* jump_to(const xmlNode* node, int xml_node_type);

        const xmlNode* jump_to_next(const xmlNode* node, int xml_node_type);
        
        const xmlNode* jump_to_children(const xmlNode* node, int xml_node_type);
        void check_node_name(const xmlNode* node, std::string name);
    };
}

void yp2::RouterFleXML::Rep::check_node_name(const xmlNode* node, std::string name)
{
    if (std::string((const char *)node->name) 
        !=  name)
        xml_dom_error(node, "expected  <" + name + ">, got ");
}

const xmlNode* yp2::RouterFleXML::Rep::jump_to_children(const xmlNode* node, int xml_node_type)
{
    node = node->children;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}
        
const xmlNode* yp2::RouterFleXML::Rep::jump_to_next(const xmlNode* node, int xml_node_type)
{
    node = node->next;
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}
        
const xmlNode* yp2::RouterFleXML::Rep::jump_to(const xmlNode* node, int xml_node_type)
{
    for (; node && node->type != xml_node_type; node = node->next)
        ;
    return node;
}

void yp2::RouterFleXML::Rep::parse_xml_config_dom()
{
    if (!m_xmlconf_doc){    
        std::cerr << "XML configuration DOM pointer empty" << std::endl;
    }
    
    const xmlNode* root = xmlDocGetRootElement(m_xmlconf_doc);
    
    if ((std::string((const char *) root->name) != "yp2")
        || (std::string((const char *)(root->ns->href)) 
            != "http://indexdata.dk/yp2/config/1")
        )
        xml_dom_error(root, 
                      "expected <yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">, got ");
    
    
    for (const struct _xmlAttr *attr = root->properties; attr; attr = attr->next)
    {
        if (std::string((const char *)attr->name) == "xmlns")
        {
            const xmlNode *val = attr->children;
            if (std::string((const char *)val->content) 
                !=  "http://indexdata.dk/yp2/config/1")
                xml_dom_error(root, 
                              "expected  xmlns=\"http://indexdata.dk/yp2/config/1\", got ");
        }  
    }
    std::cout << "processing /yp2" << std::endl;
    
    // process <start> node which is expected first element node
    const xmlNode* node = jump_to_children(root, XML_ELEMENT_NODE);
    //for (; node && node->type != XML_ELEMENT_NODE; node = node->next)
    //    ;
    
    check_node_name(node, "start");
    std::cout << "processing /yp2/start" << std::endl;
    
    // process <filters> node which is expected second element node
    node = jump_to_next(node, XML_ELEMENT_NODE);
    check_node_name(node, "filters");
    std::cout << "processing /yp2/filters" << std::endl;
    
    // process <filter> nodes  in next level
    const xmlNode* node2 = jump_to_children(node, XML_ELEMENT_NODE);
    check_node_name(node2, "filter");
    
    unsigned int filter_nr = 0;
    while(node2 && std::string((const char *)node2->name) ==  "filter"){
        filter_nr++;
        std::cout << "processing /yp2/filters/filter[" 
                  << filter_nr << "]" << std::endl;
        node2 = jump_to_next(node2, XML_ELEMENT_NODE);
    }
    
    // process <routes> node which is expected third element node
    node = jump_to_next(node, XML_ELEMENT_NODE);
    check_node_name(node, "routes");
    std::cout << "processing /yp2/routes" << std::endl;
    
    // process <route> nodes  in next level
    node2 = jump_to_children(node, XML_ELEMENT_NODE);
    check_node_name(node2, "route");
    
    unsigned int route_nr = 0;
    while(node2 && std::string((const char *)node2->name) ==  "route"){
        route_nr++;
        std::cout << "processing /yp2/routes/route[" 
                  << route_nr << "]" << std::endl;
        
        // process <filter> nodes in third level
        const xmlNode* node3 
            = jump_to_children(node2, XML_ELEMENT_NODE);
        check_node_name(node3, "filter");
        
        unsigned int filter3_nr = 0;
        while(node3 && std::string((const char *)node3->name) ==  "filter"){
            filter3_nr++;
            
            std::cout << "processing /yp2/routes/route[" 
                      << route_nr << "]/filter[" 
                      << filter3_nr << "]" << std::endl;
            
            node3 = jump_to_next(node3, XML_ELEMENT_NODE);
            
        }
        node2 = jump_to_next(node2, XML_ELEMENT_NODE);
    }
}        

void yp2::RouterFleXML::Rep::create_filter(std::string type, 
                                           const xmlDoc * xmldoc,
                                           std::string id)
{
    std::cout << "Created Filter type='" << type 
              << "' id='" << id << "'" << std::endl;
}

void yp2::RouterFleXML::Rep::xml_dom_error (const xmlNode* node, std::string msg)
{
    std::cerr << "ERROR: " << msg << " <"
              << node->name << ">"
              << std::endl;
}


yp2::RouterFleXML::Rep::Rep() : 
    m_xmlconf(""), m_xinclude(false), m_xmlconf_doc(0)
{
}

yp2::RouterFleXML::RouterFleXML(std::string xmlconf) 
    : m_p(new Rep)
{            
    LIBXML_TEST_VERSION;
    
    m_p->m_xmlconf = xmlconf;
    
    m_p->m_xmlconf_doc = xmlParseMemory(m_p->m_xmlconf.c_str(),
                                        m_p->m_xmlconf.size());
    m_p->parse_xml_config_dom();
}

yp2::RouterFleXML::~RouterFleXML()
{
    xmlFreeDoc(m_p->m_xmlconf_doc);
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
