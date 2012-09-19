/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <boost/noncopyable.hpp>
#include <string>
#include <stdexcept>

namespace metaproxy_1
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
        virtual void start() = 0;
        virtual void stop() = 0;
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

