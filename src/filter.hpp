/* $Id: filter.hpp,v 1.14 2006-01-09 21:20:15 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_HPP
#define FILTER_HPP

#include <string>
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

            virtual void configure(const xmlNode * ptr) { };
        };

        class FilterException : public std::runtime_error {
        public:
            FilterException(const std::string message)
                : std::runtime_error("FilterException: " + message){
            };
        };
    }
}

struct yp2_filter_struct {
    int ver;
    const char *type;
    yp2::filter::Base* (*creator)();
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
