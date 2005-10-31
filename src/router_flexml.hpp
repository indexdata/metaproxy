/* $Id: router_flexml.hpp,v 1.5 2005-10-31 11:59:08 marc Exp $
   Copyright (c) 2005, Index Data.

   %LICENSE%
*/

#include "router.hpp"

#include <iostream>
#include <stdexcept>
#include <map>
#include <list>

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <boost/shared_ptr.hpp>


namespace yp2 
{
    

    class RouterFleXML : public yp2::Router 
    {
    public:
        RouterFleXML(std::string xmlconf) 
            :  m_xmlconf(""), m_xinclude(false), m_xmlconf_doc(0)
            {            
                LIBXML_TEST_VERSION;
        
                m_xmlconf = xmlconf;
                m_xinclude = false;

                m_xmlconf_doc 
                    = xmlParseMemory(m_xmlconf.c_str(), m_xmlconf.size());

                parse_xml_config_dom();
            }

        ~RouterFleXML()
            {
                xmlFreeDoc(m_xmlconf_doc);
            }
    
    
    private:
        typedef std::map<std::string, boost::shared_ptr<const yp2::filter::Base> >
                IdFilterMap ;
        typedef std::list<std::string> FilterIdList;
        typedef std::map<std::string, FilterIdList > IdRouteMap ;

    private:

        std::string m_xmlconf;
        bool m_xinclude;
        xmlDoc * m_xmlconf_doc;
        IdFilterMap m_id_filter_map;
        FilterIdList m_filter_id_list;
        IdRouteMap m_id_route_map;

        //boost::shared_ptr<T> s_ptr(new T(t));


        void xml_dom_error (const xmlNode* node, std::string msg)
            {
                std::cerr << "ERROR: " << msg << " <"
                          << node->name << ">"
                          << std::endl;
            }
    
        void create_filter(std::string type, 
                           const xmlDoc * xmldoc,
                           std::string id = "")
            {
                std::cout << "Created Filter type='" << type 
                          << "' id='" << id << "'" << std::endl;
            }
        

        void parse_xml_config_dom() {
   
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
        
        
        const xmlNode* jump_to(const xmlNode* node, int xml_node_type){
            for (; node && node->type != xml_node_type; node = node->next)
                ;
            return node;
        }

        const xmlNode* jump_to_next(const xmlNode* node, int xml_node_type){
            node = node->next;
            for (; node && node->type != xml_node_type; node = node->next)
                ;
            return node;
        }
        
        const xmlNode* jump_to_children(const xmlNode* node, int xml_node_type){
            node = node->children;
            for (; node && node->type != xml_node_type; node = node->next)
                ;
            return node;
        }
        
        void check_node_name(const xmlNode* node, std::string name){
            if (std::string((const char *)node->name) 
                !=  name)
                xml_dom_error(node, "expected  <" + name + ">, got ");
        }
    
    
#if 0

        void parse_xml_config_xmlreader() {   

            xmlTextReader* reader;
            //reader->SetParserProp(libxml2.PARSER_SUBST_ENTITIES,1);
            int ret;
            //reader = xmlReaderForFile(m_xmlconf.c_str(), NULL, 0);
            reader = xmlReaderWalker(m_xmlconf_doc);
 
            if (reader == NULL) {
                std::cerr << "failed to read XML config file "
                          << std::endl
                          << m_xmlconf << std::endl;
                std::exit(1);
            }


            // root element processing
            xml_progress_deep_to_element(reader);
            if (std::string("yp2") != (const char*)xmlTextReaderConstName(reader))
                xml_error(reader, "root element must be named <yp2>");

            std::cout << "<" << xmlTextReaderConstName(reader);

            //if (xmlTextReaderHasAttributes(reader))
            //if ((!xmlTextReaderMoveToAttributeNs(reader, NULL,
            //                         (const xmlChar*)"http://indexdata.dk/yp2/config/1" )))
            if ((!xmlTextReaderMoveToFirstAttribute(reader))
                || (! xmlTextReaderIsNamespaceDecl(reader))
                || (std::string("http://indexdata.dk/yp2/config/1") 
                    != (const char*)xmlTextReaderConstValue(reader)))
                xml_error(reader, "expected root element <yp2> in namespace "
                          "'http://indexdata.dk/yp2/config/1'");

            std::cout << " " << xmlTextReaderConstName(reader) << "=\""  
                      << xmlTextReaderConstValue(reader) << "\">"  
                //<< xmlTextReaderIsNamespaceDecl(reader)
                      << std::endl;


            // start element processing
            xml_progress_deep_to_element(reader);
            if (std::string("start") != (const char*)xmlTextReaderConstName(reader)
                || !xmlTextReaderMoveToFirstAttribute(reader)
                || std::string("route") != (const char*)xmlTextReaderConstName(reader)
                )
                xml_error(reader, "start element <start route=\"route_id\"/> expected");
            std::cout << "<start " << xmlTextReaderConstName(reader) <<  "=\"" 
                      <<  xmlTextReaderConstValue(reader) << "\"/>" << std::endl;
            //<< xmlTextReaderGetAttribute(reader, (const xmlChar *)"route") 


            // filters element processing
            xml_progress_flat_to_element(reader);
        
            if (std::string("filters") != (const char*)xmlTextReaderConstName(reader)
                )
                xml_error(reader, "filters element <filters> expected");

            std::cout << "<filters>" << std::endl;
                  

            // filter element processing
            xml_progress_deep_to_element(reader);
            if (std::string("filter") != (const char*)xmlTextReaderConstName(reader)
                )
                xml_error(reader, "filter element <filter id=\"some_id\" "
                          "type=\"some_type\"/> expected");

            while (std::string("filter") == (const char*)xmlTextReaderConstName(reader)){
                std::string filter_id;
                std::string filter_type;
                if (!xmlTextReaderMoveToFirstAttribute(reader)
                    || std::string("id") != (const char*)xmlTextReaderConstName(reader))
                    xml_error(reader, "filter element <filter id=\"some_id\" "
                              "type=\"some_type\"/> expected");
                filter_id = (const char*)xmlTextReaderConstValue(reader);
                if (!xmlTextReaderMoveToNextAttribute(reader)
                    || std::string("type") != (const char*)xmlTextReaderConstName(reader))
                    xml_error(reader, "filter element <filter id=\"some_id\" "
                              "type=\"some_type\"/> expected");
                filter_type = (const char*)xmlTextReaderConstValue(reader);
                std::cout << "<filter id=\"" << filter_id 
                          << "\" type=\"" << filter_type << "\"/>" 
                          << std::endl;
                xml_progress_flat_to_element(reader);
            }

            std::cout << "</filters>" << std::endl;


            // routes element processing
            // xml_progress_flat_to_element(reader);
            if (std::string("routes") != (const char*)xmlTextReaderConstName(reader)
                )
                xml_error(reader, "routes element <routes> expected");

            std::cout << "<routes>" << std::endl;
            // route element processing
            xml_progress_deep_to_element(reader);
            if (std::string("route") != (const char*)xmlTextReaderConstName(reader)
                )
                xml_error(reader, "route element <route id=\"some_id\" "
                          "type=\"some_type\"/> expected");
            while (std::string("route") == (const char*)xmlTextReaderConstName(reader)){
                std::string route_id;
                if (!xmlTextReaderMoveToFirstAttribute(reader)
                    || std::string("id") != (const char*)xmlTextReaderConstName(reader))
                    xml_error(reader, "route element <route id=\"some_id\"/> expected");
                route_id = (const char*)xmlTextReaderConstValue(reader);


                std::cout << "<route id=\"" << route_id << "\">" << std::endl;
                std::cout << "</route>" << std::endl;
                xml_progress_flat_to_element(reader);
            }

            std::cout << "</routes>" << std::endl;

            std::cout << "</yp2>" << std::endl;

            xml_debug_print(reader);


            // freeing C xml reader libs
            xmlFreeTextReader(reader);
            if (ret != 0) {
                std::cerr << "Parsing failed of XML configuration" 
                          << std::endl 
                          << m_xmlconf << std::endl;
                std::exit(1);
            }
        }

        void xml_error ( xmlTextReader* reader, std::string msg)
            {
                std::cerr << "ERROR: " << msg << " "
                          << xmlTextReaderGetParserLineNumber(reader) << ":" 
                          << xmlTextReaderGetParserColumnNumber(reader) << " " 
                          << xmlTextReaderConstName(reader) << " "  
                          << xmlTextReaderDepth(reader) << " " 
                          << xmlTextReaderNodeType(reader) << std::endl;
            }
    
        void xml_debug_print ( xmlTextReader* reader)
            {
                // processing all other elements
                //while (xmlTextReaderMoveToElement(reader)) // reads next element ??
                //while (xmlTextReaderNext(reader)) //does not descend, keeps level 
                while (xmlTextReaderRead(reader)) // descends into all subtree nodes
                    std::cout << xmlTextReaderGetParserLineNumber(reader) << ":" 
                              << xmlTextReaderGetParserColumnNumber(reader) << " " 
                              << xmlTextReaderDepth(reader) << " " 
                              << xmlTextReaderNodeType(reader) << " "
                              << "ConstName " << xmlTextReaderConstName(reader) << " "
                              << std::endl;
            }
    
        bool xml_progress_deep_to_element(xmlTextReader* reader)
            {
                bool ret = false;
                while(xmlTextReaderRead(reader) 
                      && xmlTextReaderNodeType(reader) !=  XML_ELEMENT_NODE
                      && !( xmlTextReaderNodeType(reader) 
                            == XML_READER_TYPE_END_ELEMENT
                            && 0 == xmlTextReaderDepth(reader))
                    ) 
                    ret = true;
                return ret;
            }
    
        bool xml_progress_flat_to_element(xmlTextReader* reader)
            {
                bool ret = false;
            
                while(xmlTextReaderNext(reader) 
                      && xmlTextReaderNodeType(reader) != XML_ELEMENT_NODE
                      && !( xmlTextReaderNodeType(reader) 
                            == XML_READER_TYPE_END_ELEMENT
                            && 0 == xmlTextReaderDepth(reader))
                    ) {    
                    ret = true;
                }
                return ret;
            }
    
#endif

    };
 
};


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
