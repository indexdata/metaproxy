/* $Id: filter.hpp,v 1.7 2005-10-29 22:23:36 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_HPP
#define FILTER_HPP

#include <stdexcept>
#include <libxml/tree.h>

namespace yp2 {

    class Package;

    namespace filter {
        class Base {
        public:
            virtual ~Base(){};
            
            ///sends Package off to next Filter, returns altered Package
            virtual void process(Package & package) const = 0;

            virtual void configure(const xmlNode * ptr = 0) { };
            
            
            virtual const std::string type() const = 0;
        };
    }
    
    class FilterException : public std::runtime_error {
    public:
        FilterException(const std::string message)
            : std::runtime_error("FilterException: " + message){
        };
    };

  
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
