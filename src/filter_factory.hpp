/* $Id: filter_factory.hpp,v 1.4 2005-10-31 09:40:18 marc Exp $
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
    
    class FilterFactoryException : public std::runtime_error {
    public:
        FilterFactoryException(const std::string message)
            : std::runtime_error("FilterException: " + message){
        };
    };

        class FilterFactory {

        public:
            typedef yp2::filter::Base* (*CreateFilterCallback)();
            /// true if registration ok

            FilterFactory(){};

            bool add_creator(std::string fi, CreateFilterCallback cfc);
            /// true if unregistration ok

            bool drop_creator(std::string fi);
            
            /// factory create method

            yp2::filter::Base* create(std::string fi);
            
        private:
            typedef std::map<std::string, CreateFilterCallback> CallbackMap;
            CallbackMap m_fcm;

        private:
            /// disabled because class is singleton
            FilterFactory(const FilterFactory &);
            
            /// disabled because class is singleton
            FilterFactory& operator=(const FilterFactory &);
        };
        
    }
    
    bool yp2::filter::FilterFactory::add_creator(std::string fi,
                                    CreateFilterCallback cfc)
    {
        return m_fcm.insert(CallbackMap::value_type(fi, cfc)).second;
    }
    
    
    bool yp2::filter::FilterFactory::drop_creator(std::string fi)
    {
        return m_fcm.erase(fi) == 1;
    }
    
    yp2::filter::Base* yp2::filter::FilterFactory::create(std::string fi)
    {
        CallbackMap::const_iterator i = m_fcm.find(fi);
        
        if (i == m_fcm.end()){
            std::string msg = "filter type '" + fi + "' not found";
            throw yp2::filter::FilterFactoryException(msg);
        }
        // call create function
        return (i->second());
    }
    

  
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
