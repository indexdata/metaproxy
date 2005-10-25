/* $Id: ex_libxml2_conf.cpp,v 1.3 2005-10-25 13:42:44 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <libxml/xmlreader.h>

class Configuration 
{
public:
    Configuration(int argc, char **argv)
        {            
            m_config = "";
            m_duration = 0;
            m_xinclude = false;
            m_xml_conf_doc = 0;
   
            parse_command_line(argc, argv);
            parse_xml_config();
        }
    
public:
    std::string config()
        {
         return m_config;
        }
    int duration()
        {
            return m_duration;
        }
    
    
private:
    std::string m_config;
    int m_duration;      
    bool m_xinclude;
    xmlDoc * m_xml_conf_doc;
    
    
    void parse_command_line(int argc, char **argv)
        {
            po::options_description generic("Generic options");
            generic.add_options()
                ("help,h", "produce help message")
                ("version,v", "version number")
                ("xinclude,x", "allow Xinclude on XML config files")
                ;
            
            po::options_description config("Configuration options");
         config.add_options()
            ("config,c", po::value<std::string>(&m_config)->default_value("../etc/config1.xml"),
             "config XML file path (string)")
            ("duration,d", po::value<int>(&m_duration)->default_value(0),
             "number of seconds for server to exist (int)")
            //("commands", po::value< std::vector<std::string> >(), 
            //  "listener ports (string)")
            ;
         
         //po::positional_options_description p;
         // p.add("port", -1);
         
         po::options_description cmdline_options;
         cmdline_options.add(generic).add(config);
         po::variables_map vm;        

         try 
         {
            //po::store(po::command_line_parser(argc, argv).
            //       options(cmdline_options).positional(p).run(), vm);
            //po::store(po::command_line_parser(argc, argv).
            //     options(cmdline_options).run(), vm);
            po::store(po::parse_command_line(argc, argv,  cmdline_options), vm);
            po::notify(vm);    
         }
         catch ( po::invalid_command_line_syntax &e) {      
            std::cerr << "ex_libxml2_conf error: " << e.what() << std::endl;
            std::cerr << generic << config << std::endl;
            std::exit(1);
        }
         catch ( po::invalid_option_value &e) {      
            //std::cerr << "ex_libxml2_conf error: " << e.what() << std::endl;
            std::cerr << "invalid option value" << std::endl;
            std::cerr << generic << config << std::endl;
            std::exit(1);
         }
         catch ( po::unknown_option &e) {      
            std::cerr << "ex_libxml2_conf error: " << e.what() << std::endl;
            std::cerr << generic << config << std::endl;
           std::exit(1);
         }
         
         std::cout << "ex_libxml2_conf ";
         
         if (vm.count("help")) {
            std::cout << "--help" << std::endl;
            std::cout << generic << config << std::endl;
            std::exit(0);
         }
         
        if (vm.count("version")) {
           std::cout << "--version" << std::endl;
           std::exit(0);
        }
         
        if (vm.count("xinclude")) {
           std::cout << "--xinclude" << std::endl;
           m_xinclude = true;
        }
        
        if (vm.count("duration")) {
           std::cout << "--duration " 
                     << vm["duration"].as<int>() << " ";
        }
        if (vm.count("config")) {
           std::cout << "--config " 
                     << vm["config"].as<std::string>() << " ";
        }
        
        std::cout << std::endl;

        //if (vm.count("port"))
        //{
        //    std::vector<std::string> ports = 
        //       vm["port"].as< std::vector<std::string> >();
        //    
        //    for (size_t i = 0; i<ports.size(); i++)
        //       std::cout << "port " << i << " " << ports[i] << std::endl;
            
        //}

        }
    
    void parse_xml_config() {   
        LIBXML_TEST_VERSION

        xmlTextReader* reader;
        //reader->SetParserProp(libxml2.PARSER_SUBST_ENTITIES,1);
        int ret;
        reader = xmlReaderForFile(m_config.c_str(), NULL, 0);
 
        if (reader == NULL) {
            std::cerr << "failed to open XML config file " 
                      << m_config << std::endl;
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

            std::cout << "progress_flat " << xml_progress_flat_to_element(reader) << std::endl;

        }


        std::cout << "NOW: "<< xmlTextReaderGetParserLineNumber(reader) << ":" 
                  << xmlTextReaderGetParserColumnNumber(reader) << " " 
                  << xmlTextReaderDepth(reader) << " " 
                  << xmlTextReaderNodeType(reader) << " "
                  << "ConstName " << xmlTextReaderConstName(reader) << " "
                  << std::endl;
        

        std::cout << "</routes>" << std::endl;

        
        
        // processing all other elements
        //while ((ret = xmlTextReaderMoveToElement(reader))) // reads next element ??
        //while ((ret = xmlTextReaderNext(reader))) //does not descend, keeps level 
        while ((ret = xmlTextReaderRead(reader))) // descends into all subtree nodes
            std::cout << xmlTextReaderGetParserLineNumber(reader) << ":" 
                      << xmlTextReaderGetParserColumnNumber(reader) << " " 
                      << xmlTextReaderDepth(reader) << " " 
                      << xmlTextReaderNodeType(reader) << " "
                      << "ConstName " << xmlTextReaderConstName(reader) << " "  
                //<< "Prefix " << xmlTextReaderPrefix(reader) << "\n"  
                //<< "XmlLang " << xmlTextReaderXmlLang(reader) << "\n"  
                //<< "NamespaceUri " << xmlTextReaderNamespaceUri(reader) << "\n"  
                //<< "BaseUri"  << xmlTextReaderBaseUri(reader) << "\n"  
                      << std::endl;


        xmlFreeTextReader(reader);
        if (ret != 0) {
            std::cerr << "Parsing failed of XML config file " 
                      << m_config << std::endl;
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
    
    bool xml_progress_deep_to_element(xmlTextReader* reader)
        {
            bool ret = false;
            while(xmlTextReaderRead(reader) 
                  && xmlTextReaderNodeType(reader) !=  XML_ELEMENT_NODE
                ) 
                ret = true;
            return ret;
        }
    
    bool xml_progress_flat_to_element(xmlTextReader* reader)
        {
            bool ret = false;
            //int depth = xmlTextReaderDepth(reader);
            
            while(xmlTextReaderNext(reader) 
                  //&& depth == xmlTextReaderDepth(reader)
                  && xmlTextReaderNodeType(reader) != XML_ELEMENT_NODE
                  //&& xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT
                ) {    
                ret = true;
                std::cout << xmlTextReaderDepth(reader) << " " 
                          << xmlTextReaderNodeType(reader) << std::endl;
                
            }
            return ret;
        }
    

};



int main(int argc, char **argv)
{
   //try 
   //{

   Configuration conf(argc, argv);

   std::cout << "config " << conf.config() << std::endl;
   std::cout << "duration " << conf.duration() << std::endl;
   



        // }
        //catch ( ... ) {
        //std::cerr << "Unknown Exception" << std::endl;
        //throw();
        //std::exit(1);
        //}
   std::exit(0);
}




/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
