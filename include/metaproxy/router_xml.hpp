/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

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

#ifndef ROUTER_XML_HPP
#define ROUTER_XML_HPP

#include <metaproxy/router.hpp>

#include <libxml/tree.h>
#include <boost/scoped_ptr.hpp>

namespace metaproxy_1
{
    class RouterXML : public metaproxy_1::Router
    {
        class Rep;
        class Route;
        class Pos;
    public:
        RouterXML(xmlDocPtr doc,
                  bool test_only, const char *file_include_path);
        RouterXML(std::string xmlconf,
                  bool test_only);

        ~RouterXML();

        virtual RoutePos *createpos() const;
        void start();
        void stop(int signo);
    private:
        boost::scoped_ptr<Rep> m_p;
    };

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

