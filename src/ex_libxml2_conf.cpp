/* $Id: ex_libxml2_conf.cpp,v 1.1 2005-10-24 13:31:36 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>
namespace po = boost::program_options;


class Config 
{
   public:
      Config(int argc, char **argv)
      {
         
         po::options_description generic("Generic options");
         generic.add_options()
            ("help,h", "produce help message")
            ("version,v", "version number")
            ;

         po::options_description config("Config options");
         config.add_options()
            ("config,c", po::value<std::string>(&m_config)->default_value("ex_libxml2_conf.xml"),
             "config XML file path (string)")
            ("duration,d", po::value<int>(&m_duration)->default_value(3),
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
      //xmlDoc * m_conf_doc;
 
      bool load_xml_doc()
      {
         return true;
      }
      
};



int main(int argc, char **argv)
{
   //try 
   //{

   Config conf(argc, argv);

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
