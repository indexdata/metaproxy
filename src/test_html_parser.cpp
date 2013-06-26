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
#include <iostream>
#include <stdexcept>

#include "html_parser.hpp"
#include <metaproxy/util.hpp>

#include <boost/lexical_cast.hpp>

#include <yaz/log.h>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

class MyEvent : public mp::HTMLParserEvent
{
public:
    std::string out;
    void openTagStart(const char *tag, int tag_len) {
        out += "<";
        out.append(tag, tag_len);
    } 
    
    void attribute(const char *tag, int tag_len,
                   const char *attr, int attr_len,
                   const char *value, int val_len) {
        out += " ";
        out.append(attr, attr_len);
        out += "=\"";
        out.append(value, val_len);
        out += "\"";
    }
    void anyTagEnd(const char *tag, int tag_len, int close_it) {
        if (close_it)
            out += "/";
        out += ">";
    }
    void closeTag(const char *tag, int tag_len) {
        out += "</";
        out.append(tag, tag_len);
    }
    void text(const char *value, int len) {
        out.append(value, len);
    }
};


BOOST_AUTO_TEST_CASE( test_html_parser_1 )
{
    try
    {
        mp::HTMLParser hp;
        const char* html = 
            "<html><body><a t1=v1 t2='v2' t3=\"v3\">some text</a>"
            "<hr><table ></table  ><a href=\"x\"/></body></html>";
        const char* expected = 
            "<html><body><a t1=\"v1\" t2=\"v2\" t3=\"v3\">some text</a>"
            "<hr><table></table><a href=\"x\"/></body></html>";
        MyEvent e;
        hp.parse(e, html);

        std::cout << "Expected" << std::endl;
        std::cout << expected << std::endl;
        std::cout << "Got" << std::endl;
        std::cout << e.out << std::endl;
        BOOST_CHECK_EQUAL(std::string(expected), e.out);
    }
    catch (std::exception & e) 
    {
        std::cout << e.what();
        std::cout << std::endl;
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_html_parser_2 )
{
    try
    {
        mp::HTMLParser hp;
        const char* html = 
            "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
            "<HTML>\n"
            " <HEAD>\n"
            "  <TITLE>YAZ 4.2.60</TITLE>\n"
            " </HEAD>\n"
            " <BODY>\n"
            "  <P><A HREF=\"http://www.indexdata.com/yaz/\">YAZ</A> 4.2.60</P>\n"
            "  <P>Error: 404</P>\n"
            "  <P>Description: Not Found</P>\n"
            " </BODY>\n"
            "</HTML>";

        const char* expected = html;
        MyEvent e;
        hp.parse(e, html);

        std::cout << "Expected" << std::endl;
        std::cout << expected << std::endl;
        std::cout << "Got" << std::endl;
        std::cout << e.out << std::endl;

        BOOST_CHECK_EQUAL(std::string(expected), e.out);
    }
    catch (std::exception & e) 
    {
        std::cout << e.what();
        std::cout << std::endl;
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

