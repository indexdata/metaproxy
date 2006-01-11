/* $Id: router.hpp,v 1.10 2006-01-11 11:51:50 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <boost/noncopyable.hpp>
#include <string>
#include <stdexcept>

namespace yp2 
{
    namespace filter {
        class Base;
    }
    class RoutePos;
    
    class RouterException : public std::runtime_error {
    public:
        RouterException(const std::string message)
            : std::runtime_error("RouterException: " + message){};
    };
      
    class Router : boost::noncopyable {
    public:
        Router(){};
        virtual ~Router(){};

        virtual RoutePos *createpos() const = 0;
    };

    class RoutePos : boost::noncopyable {
    public:
        virtual const filter::Base *move(const char *route) = 0;
        virtual RoutePos *clone() = 0;
        virtual ~RoutePos() {};
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
