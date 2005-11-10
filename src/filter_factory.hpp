/* $Id: filter_factory.hpp,v 1.6 2005-11-10 23:10:42 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_FACTORY_HPP
#define FILTER_FACTORY_HPP

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "filter.hpp"


namespace yp2 {

    class FilterFactoryException : public std::runtime_error {
    public:
        FilterFactoryException(const std::string message);
    };
    
    class FilterFactory : public boost::noncopyable
    {
        typedef yp2::filter::Base* (*CreateFilterCallback)();
        typedef std::map<std::string, CreateFilterCallback> CallbackMap;

        class Rep;
    public:
        /// true if registration ok
        
        FilterFactory();
        ~FilterFactory();

        bool add_creator(std::string fi, CreateFilterCallback cfc);
        /// true if unregistration ok
        
        bool drop_creator(std::string fi);
        
        /// factory create method
        
        yp2::filter::Base* create(std::string fi);
        
    private:
        boost::scoped_ptr<Rep> m_p;
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
