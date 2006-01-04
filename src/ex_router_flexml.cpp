/* $Id: ex_router_flexml.cpp,v 1.5 2006-01-04 14:30:51 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include <iostream>
#include <stdexcept>

#include "config.hpp"
#include "filter.hpp"
#include "router_flexml.hpp"

int main(int argc, char **argv)
{
    try 
    {
        
       std::string xmlconf = "<?xml version=\"1.0\"?>\n"
           "<yp2 xmlns=\"http://indexdata.dk/yp2/config/1\">\n"
           "<start route=\"start\"/>\n"
           "<filters>\n"
           "<filter id=\"front_default\" type=\"frontend-net\">\n"
            "<port>210</port>\n"
           "</filter>\n"
           "<filter id=\"log_cout\" type=\"log\">\n"
           "<logfile>mylog.log</logfile>\n"
           "</filter>\n"
           "</filters>\n"
            "<routes>\n"  
           "<route id=\"start\">\n"
           "<filter refid=\"front_default\"/>\n"
           "<filter refid=\"log_cout\"/>\n"
           "</route>\n"
            "</routes>\n"
           "</yp2>\n";

       yp2::FactoryFilter factory;
       yp2::RouterFleXML rflexml(xmlconf, factory);
       
       
       
   }
    catch ( ... ) {
        std::cerr << "Unknown Exception" << std::endl;
        throw;
        std::exit(1);
    }
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
