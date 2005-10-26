/* $Id: router.hpp,v 1.4 2005-10-26 10:21:03 marc Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <stdexcept>
#include <list>

namespace yp2 {
    namespace filter {
        class Base;
    }
    class Package;
    
    class RouterException : public std::runtime_error {
    public:
        RouterException(const std::string message)
            : std::runtime_error("RouterException: " + message){};
    };
  
    
    class Router {
    public:
        Router(){};
        virtual ~Router(){};

        /// determines next Filter to use from current Filter and Package
        virtual const filter::Base *move(const filter::Base *filter,
                                   const Package *package) const {
            return 0;
        };

        /// re-read configuration of routing tables
        //virtual void configure(){};

        /// add routing rule expressed as Filter to Router
        //virtual Router & rule(const filter::Base &filter){
        //    return *this;
        //}
    private:
        /// disabled because class is singleton
        Router(const Router &);

        /// disabled because class is singleton
        Router& operator=(const Router &);
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
