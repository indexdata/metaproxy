/* $Id: router_flexml.hpp,v 1.12 2006-01-09 13:53:13 adam Exp $
   Copyright (c) 2005, Index Data.

   %LICENSE%
*/

#include "router.hpp"

#include "factory_filter.hpp"

#include <stdexcept>

#include <boost/scoped_ptr.hpp>

namespace yp2 
{
    class RouterFleXML : public yp2::Router 
    {
        class Rep;
        class Route;
        class Pos;
    public:
        RouterFleXML(std::string xmlconf, yp2::FactoryFilter &factory);
        RouterFleXML(xmlDocPtr doc, yp2::FactoryFilter &factory);
        
        ~RouterFleXML();

        virtual RoutePos *createpos() const;
        class XMLError : public std::runtime_error {
        public:
            XMLError(const std::string msg) :
                std::runtime_error("XMLError : " + msg) {} ;
        };
    private:
        boost::scoped_ptr<Rep> m_p;
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
