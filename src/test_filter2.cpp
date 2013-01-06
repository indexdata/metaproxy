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

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <metaproxy/filter.hpp>
#include "router_chain.hpp"
#include <metaproxy/package.hpp>

#include <iostream>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

namespace mp = metaproxy_1;

class FilterConstant: public mp::filter::Base {
public:
    FilterConstant() : m_constant(1234) { };
    void process(mp::Package & package) const {
	package.move();
    };
    void configure(const xmlNode* ptr, bool test_only, const char *path);
    int get_constant() const { return m_constant; };
private:
    const xmlNode *m_ptr;
    int m_constant;
};


void FilterConstant::configure(const xmlNode* ptr, bool test_only,
                               const char *path)
{
    m_ptr = ptr;

    BOOST_CHECK_EQUAL (ptr->type, XML_ELEMENT_NODE);
    BOOST_CHECK_EQUAL(std::string((const char *) ptr->name), "filter");

    const struct _xmlAttr *attr;

    for (attr = ptr->properties; attr; attr = attr->next)
    {
        BOOST_CHECK_EQUAL( std::string((const char *)attr->name), "type");
        const xmlNode *val = attr->children;
        BOOST_CHECK_EQUAL(val->type, XML_TEXT_NODE);
        BOOST_CHECK_EQUAL(std::string((const char *)val->content), "constant");
    }
    const xmlNode *p = ptr->children;
    for (; p; p = p->next)
    {
        if (p->type != XML_ELEMENT_NODE)
            continue;

        BOOST_CHECK_EQUAL (p->type, XML_ELEMENT_NODE);
        BOOST_CHECK_EQUAL(std::string((const char *) p->name), "value");

        const xmlNode *val = p->children;
        BOOST_CHECK(val);
        if (!val)
            continue;

        BOOST_CHECK_EQUAL(val->type, XML_TEXT_NODE);
        BOOST_CHECK_EQUAL(std::string((const char *)val->content), "2");

        m_constant = atoi((const char *) val->content);
    }
}

// This filter dose not have a configure function

class FilterDouble: public mp::filter::Base {
public:
    void process(mp::Package & package) const {
	package.move();
    };
    void configure(const xmlNode * ptr, bool test_only,
                   const char *path) { };
};


BOOST_AUTO_TEST_CASE( testfilter2_1 )
{
    try {
	FilterConstant fc;
	FilterDouble fd;

	{
	    mp::RouterChain router1;

	    // test filter set/get/exception
	    router1.append(fc);

	    router1.append(fd);

            mp::Session session;
            mp::Origin origin;
	    mp::Package pack(session, origin);

	    pack.router(router1).move();

            //BOOST_CHECK_EQUAL(pack.data(), 2468);

        }

        {
	    mp::RouterChain router2;

	    router2.append(fd);
	    router2.append(fc);

            mp::Session session;
            mp::Origin origin;
	    mp::Package pack(session, origin);

            pack.router(router2).move();

            //BOOST_CHECK_EQUAL(pack.data(), 1234);

	}

    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ...) {
        BOOST_CHECK (false);
    }

}

BOOST_AUTO_TEST_CASE( testfilter2_2 )
{
    try {
	FilterConstant fc;
        BOOST_CHECK_EQUAL(fc.get_constant(), 1234);

        mp::filter::Base *base = &fc;

        std::string some_xml = "<?xml version=\"1.0\"?>\n"
            "<filter type=\"constant\">\n"
            " <value>2</value>\n"
            "</filter>";

        // std::cout << some_xml  << std::endl;

        xmlDocPtr doc = xmlParseMemory(some_xml.c_str(), some_xml.size());

        BOOST_CHECK(doc);

        if (doc)
        {
            xmlNodePtr root_element = xmlDocGetRootElement(doc);

            base->configure(root_element, true, 0);

            xmlFreeDoc(doc);
        }

        BOOST_CHECK_EQUAL(fc.get_constant(), 2);
    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch ( ...) {
        BOOST_CHECK (false);
    }

}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

