/* $Id: filter.hpp,v 1.16 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
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
