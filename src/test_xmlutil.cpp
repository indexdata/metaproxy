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

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include <metaproxy/xmlutil.hpp>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

#include <yaz/zgdu.h>
#include <yaz/otherinfo.h>
#include <yaz/oid_db.h>

using namespace boost::unit_test;
namespace mp = metaproxy_1;
namespace mp_xml = metaproxy_1::xml;

BOOST_AUTO_TEST_CASE( url_recipe )
{
    try 
    {
        const char *xml_text = 
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<pz:record xmlns:pz=\"http://www.indexdata.com/pazpar2/1.0\""
            " xmlns:tmarc=\"http://www.indexdata.com/turbomarc\">\n"
            "<pz:metadata type=\"id\">   11224466 </pz:metadata>\n"
            "<pz:metadata type=\"oclc-number\"/>\n"
            "<pz:metadata type=\"lccn\">   11224466 </pz:metadata>\n"
            "<pz:metadata type=\"author\">Jack Collins</pz:metadata>\n"
            "<pz:metadata type=\"author-title\"/>\n"
            "<pz:metadata type=\"author-date\"/>\n"
            "<pz:metadata type=\"date\"/>\n"
            "<pz:metadata type=\"title\">How to program a computer</pz:metadata>\n"
            "<pz:metadata type=\"publication-place\">Penguin</pz:metadata>\n"
            "<pz:metadata type=\"has-fulltext\">no</pz:metadata>\n"
            "</pz:record>\n";
        xmlDoc *doc = xmlParseMemory(xml_text, strlen(xml_text));
        BOOST_CHECK(doc);
        if (doc)
        {
            std::string res;

            res = mp_xml::url_recipe_handle(doc, "abc");
            BOOST_CHECK(!res.compare("abc"));

            res = mp_xml::url_recipe_handle(doc, "${has-fulltext[no/yes]}");
            BOOST_CHECK(!res.compare("yes"));

            res = mp_xml::url_recipe_handle(doc, "<${has-fulltext[no/yes]}>");
            BOOST_CHECK(!res.compare("<yes>"));

            res = mp_xml::url_recipe_handle(doc, "${has-fulltext[no]}");
            BOOST_CHECK(!res.compare(""));

            res = mp_xml::url_recipe_handle(doc, "${has-fulltext[no/]}");
            BOOST_CHECK(!res.compare(""));

            res = mp_xml::url_recipe_handle(doc, "${has-fulltext[n/]}");
            BOOST_CHECK(!res.compare("o"));

            res = mp_xml::url_recipe_handle(doc, "${has-fulltext}");
            BOOST_CHECK(!res.compare("no"));

            res = mp_xml::url_recipe_handle(doc, "%{has-fulltext}");
            BOOST_CHECK(!res.compare("no"));

            res = mp_xml::url_recipe_handle(
                doc, "http://sever.com?title=${md-title[\\s+/+/g]}");
            BOOST_CHECK(!res.compare("http://sever.com?title=How+to+program+a+computer"));

            res = mp_xml::url_recipe_handle(
                doc, "http://sever.com?title=%{md-title}");
            BOOST_CHECK(!res.compare("http://sever.com?title=How%20to%20program%20a%20computer"));


            res = mp_xml::url_recipe_handle(doc, "${md-id[2/1]}");
            BOOST_CHECK(!res.compare("   11124466 "));

            res = mp_xml::url_recipe_handle(doc, "${md-id[2/1/g]}");
            BOOST_CHECK(!res.compare("   11114466 "));

            xmlFreeDoc(doc);
        }
    }
    catch ( std::runtime_error &e) {
        std::cout << "std::runtime error: " << e.what() << std::endl;
        BOOST_CHECK(false);
    }
    catch ( ... ) {
        std::cout << "unknown exception" << std::endl;
        BOOST_CHECK(false);
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

