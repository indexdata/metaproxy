/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef FACTORY_FILTER_HPP
#define FACTORY_FILTER_HPP

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <metaproxy/filter.hpp>

namespace metaproxy_1 {
    class FactoryFilter : public boost::noncopyable
    {
        typedef metaproxy_1::filter::Base* (*CreateFilterCallback)();

        class Rep;
    public:
        /// true if registration ok
        
        FactoryFilter();
        ~FactoryFilter();

        bool add_creator(const std::string &fi, CreateFilterCallback cfc);
        
        bool drop_creator(std::string fi);
        
        metaproxy_1::filter::Base* create(std::string fi);
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

