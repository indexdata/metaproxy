/* This file is part of Metaproxy.
   Copyright (C) 2005-2009 Index Data

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

#ifndef FILTER_HPP
#define FILTER_HPP

#include <string>
#include <stdexcept>
#include <libxml/tree.h>
#include "xmlutil.hpp"

namespace metaproxy_1 {

    class Package;

    namespace filter {
        class Base {
        public:
            virtual ~Base(){};
            
            ///sends Package off to next Filter, returns altered Package
            virtual void process(Package & package) const = 0;

            /// configuration during filter load 
            virtual void configure(const xmlNode * ptr, bool test_only);
        };

        class FilterException : public std::runtime_error {
        public:
            FilterException(const std::string message)
                : std::runtime_error("FilterException: " + message){
            };
        };
    }
}

struct metaproxy_1_filter_struct {
    int ver;
    const char *type;
    metaproxy_1::filter::Base* (*creator)();
};

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

