/* $Id: filter.hpp,v 1.18 2006-09-29 09:48:35 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
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
            virtual void configure(const xmlNode * ptr);
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
