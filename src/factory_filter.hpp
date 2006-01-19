/* $Id: factory_filter.hpp,v 1.3 2006-01-19 09:41:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FACTORY_FILTER_HPP
#define FACTORY_FILTER_HPP

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    class FactoryFilter : public boost::noncopyable
    {
        typedef yp2::filter::Base* (*CreateFilterCallback)();

        class Rep;
    public:
        /// true if registration ok
        
        FactoryFilter();
        ~FactoryFilter();

        bool add_creator(std::string fi, CreateFilterCallback cfc);
        
        bool drop_creator(std::string fi);
        
        yp2::filter::Base* create(std::string fi);
        bool exist(std::string fi);
    
        bool add_creator_dl(const std::string &fi, const std::string &path);

        bool have_dl_support();

        class NotFound : public std::runtime_error {
        public:
            NotFound(const std::string msg);
        };
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
