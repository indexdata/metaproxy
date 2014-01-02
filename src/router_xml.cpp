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

#include "config.hpp"
#include <metaproxy/router_xml.hpp>
#include "router_flexml.hpp"
#include "factory_static.hpp"

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    class RouterXML::Rep {
    public:
        Rep(xmlDocPtr, bool, const char *);
        Rep(std::string, bool);
        ~Rep();
        FactoryStatic m_factory;
        boost::scoped_ptr<Router> m_flexml;
    };
}

mp::RouterXML::Rep::Rep(xmlDocPtr doc, bool test_only,
                        const char *include_path)
    : m_factory(),
      m_flexml(new RouterFleXML(doc, m_factory, test_only, include_path))
{
}

mp::RouterXML::Rep::Rep(std::string xmlconf, bool test_only)
    : m_factory(),
      m_flexml(new RouterFleXML(xmlconf, m_factory, test_only))
{
}

mp::RouterXML::Rep::~Rep()
{
}

mp::RouterXML::RouterXML(xmlDocPtr doc,
                         bool test_only, const char *file_include_path)
    : m_p(new Rep(doc, test_only, file_include_path))
{
}

mp::RouterXML::RouterXML(std::string xmlconf, bool test_only)
    : m_p(new Rep(xmlconf, test_only))
{
}

mp::RouterXML::~RouterXML()
{
}

mp::RoutePos *mp::RouterXML::createpos() const
{
    return m_p->m_flexml->createpos();
}

void mp::RouterXML::start()
{
    m_p->m_flexml->start();
}

void mp::RouterXML::stop(int signo)
{
    m_p->m_flexml->stop(signo);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

