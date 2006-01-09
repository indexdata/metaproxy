/* $Id: router.hpp,v 1.8 2006-01-09 13:43:59 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <boost/noncopyable.hpp>
#include <string>
#include <stdexcept>

#define ROUTE_POS 1

namespace yp2 
{
    namespace filter {
        class Base;
    }
#if ROUTE_POS
    class RoutePos;
#else
    class Package;
#endif
    
    class RouterException : public std::runtime_error {
    public:
        RouterException(const std::string message)
            : std::runtime_error("RouterException: " + message){};
    };
      
    class Router : boost::noncopyable {
    public:
        Router(){};
        virtual ~Router(){};

#if ROUTE_POS
        virtual RoutePos *createpos() const = 0;
#else
        /// determines next Filter to use from current Filter and Package
        virtual const filter::Base *move(const filter::Base *filter,
                                         const Package *package) const = 0;
#endif
    };


#if ROUTE_POS
    class RoutePos {
    public:
        virtual const filter::Base *move() = 0;
        virtual RoutePos *clone() = 0;
        virtual ~RoutePos() {};
    };
#endif

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
