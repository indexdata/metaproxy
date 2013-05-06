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

#include "filter_http_client.hpp"
#include "filter_http_rewrite.hpp"
#include <metaproxy/util.hpp>
#include "router_chain.hpp"
#include <metaproxy/package.hpp>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;


BOOST_AUTO_TEST_CASE( test_filter_rewrite_1 )
{
    try
    {
        mp::filter::HttpRewrite fhr;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_rewrite_2 )
{
    try
    {
        mp::RouterChain router;

        mp::filter::HttpRewrite fhr;
        
        mp::filter::HttpRewrite::spair_vec vec_req;
        vec_req.push_back(std::make_pair(
        "(?<proto>http\\:\\/\\/s?)(?<pxhost>[^\\/?#]+)\\/(?<pxpath>[^\\/]+)"
        "\\/(?<host>[^\\/]+)(?<path>.*)",
        "${proto}${host}${path}"
        ));
        vec_req.push_back(std::make_pair(
        "(?:Host\\: )(.*)",
        "Host: localhost"
        ));

        mp::filter::HttpRewrite::spair_vec vec_res;
        vec_res.push_back(std::make_pair(
        "(?<proto>http\\:\\/\\/s?)(?<host>[^\\/?#]+)\\/(?<path>[^ >]+)",
        "http://${pxhost}/${pxpath}/${host}/${path}"
        ));
        
        fhr.configure(vec_req, vec_res);

        mp::filter::HTTPClient hc;
        
        router.append(fhr);
        router.append(hc);

        // create an http request
        mp::Package pack;

        mp::odr odr;
        Z_GDU *gdu_req = z_get_HTTP_Request_uri(odr, 
        "http://proxyhost/proxypath/localhost:80/~jakub/targetsite.php", 0, 1);

        pack.request() = gdu_req;

        //feed to the router
        pack.router(router).move();

        //analyze the response
        Z_GDU *gdu_res = pack.response().get();
        BOOST_CHECK(gdu_res);
        BOOST_CHECK_EQUAL(gdu_res->which, Z_GDU_HTTP_Response);
        
        Z_HTTP_Response *hres = gdu_res->u.HTTP_Response;
        BOOST_CHECK(hres);

    }
    catch (std::exception & e) {
        std::cout << e.what();
        std::cout << std::endl;
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_rewrite_3 )
{
    try
    {
        std::string xmlconf =
            "<?xml version='1.0'?>\n"
            "<filter xmlns='http://indexdata.com/metaproxy'\n"
            "        id='rewrite1' type='http_rewrite'>\n"
            " <request>\n"
            "   <rewrite from='"
    "(?&lt;proto>http\\:\\/\\/s?)(?&lt;pxhost>[^\\/?#]+)\\/(?&lt;pxpath>[^\\/]+)"
    "\\/(?&lt;host>[^\\/]+)(?&lt;path>.*)'\n"
            "            to='${proto}${host}${path}' />\n"
            "   <rewrite from='(?:Host\\: )(.*)'\n"
            "            to='Host: localhost' />\n" 
            " </request>\n"
            " <response>\n"
            "   <rewrite from='"
    "(?&lt;proto>http\\:\\/\\/s?)(?&lt;host>[^\\/?#]+)\\/(?&lt;path>[^ >]+)'\n"
            "            to='http://${pxhost}/${pxpath}/${host}/${path}' />\n" 
            " </response>\n"
            "</filter>\n"
            ;

        std::cout << xmlconf;

        // reading and parsing XML conf
        xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(), xmlconf.size());
        BOOST_CHECK(doc);
        xmlNode *root_element = xmlDocGetRootElement(doc);
        mp::filter::HttpRewrite fhr; 
        fhr.configure(root_element, true, "");
        xmlFreeDoc(doc);

        mp::filter::HTTPClient hc;
        
        mp::RouterChain router;
        router.append(fhr);
        router.append(hc);

        // create an http request
        mp::Package pack;

        mp::odr odr;
        Z_GDU *gdu_req = z_get_HTTP_Request_uri(odr, 
        "http://proxyhost/proxypath/localhost:80/~jakub/targetsite.php", 0, 1);

        pack.request() = gdu_req;

        //feed to the router
        pack.router(router).move();

        //analyze the response
        Z_GDU *gdu_res = pack.response().get();
        BOOST_CHECK(gdu_res);
        BOOST_CHECK_EQUAL(gdu_res->which, Z_GDU_HTTP_Response);
        
        Z_HTTP_Response *hres = gdu_res->u.HTTP_Response;
        BOOST_CHECK(hres);

    }
    catch (std::exception & e) {
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

