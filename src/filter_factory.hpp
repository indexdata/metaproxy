/* $Id: filter_factory.hpp,v 1.1 2005-10-28 10:35:30 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_FACTORY_HPP
#define FILTER_FACTORY_HPP

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

#include "config.hpp"
#include "filter.hpp"


namespace yp2 {

    namespace filter {
        class FilterFactory {

#if 0
        public:
            typedef yp2::filter::Base* (*CreateFilterCallback)();
            /// true if registration ok
            bool register_filter(std::string fi, CreateFilterCallback cfc);
            /// true if unregistration ok
            bool unregister_filter(std::string fi);
            /// factory create method
            yp2::filter::Base* create(std::string fi);
            
        private:
            typedef std::map<std::string, CreateFilterCallback> CallbackMap;

#endif      

        };
    }
    
    class FilterFactoryException : public std::runtime_error {
    public:
        FilterFactoryException(const std::string message)
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
