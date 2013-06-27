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
#include <metaproxy/router_chain.hpp>
#include <metaproxy/package.hpp>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <yaz/log.h>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;
/*
 * The global testconfig is commented out, as it won't even compile
 * on old Centos5 machines
struct TestConfig {
    TestConfig()
    {
        std::cout << "global setup\n";
        yaz_log_init_level(YLOG_ALL);
    }
    ~TestConfig()
    {
        std::cout << "global teardown\n";
    }
};

BOOST_GLOBAL_FIXTURE( TestConfig );
*/

BOOST_AUTO_TEST_CASE( test_filter_rewrite_1 )
{
    try
    {
        std::cout << "Running non-xml config test case" << std::endl;
        mp::RouterChain router;
        mp::filter::HttpRewrite fhr;

        std::string xmlconf =
            "<?xml version='1.0'?>\n"
            "<filter xmlns='http://indexdata.com/metaproxy'\n"
            "        id='rewrite1' type='http_rewrite'>\n"
            " <request verbose=\"1\">\n"
            "   <rule name=\"null\"/>\n"
            "   <rule name=\"url\">\n"
            "     <rewrite from='"
    "(?&lt;proto>https?://)(?&lt;pxhost>[^ /?#]+)/(?&lt;pxpath>[^ /]+)"
    "/(?&lt;host>[^ /]+)(?&lt;path>[^ ]*)'\n"
            "            to='${proto}${host}${path}' />\n"
            "     <rewrite from='(?:Host: )(.*)'\n"
            "            to='Host: ${host}' />\n"
            "   </rule>\n"
            "  <within header=\"link\" rule=\"null\"/>\n"
            "  <within reqline=\"1\" rule=\"url\"/>\n"
            " </request>\n"
            " <response verbose=\"1\">\n"
            "   <rule name=\"null\"/>\n"
            "   <rule name=\"url\">\n"
            "     <rewrite from='"
    "(?&lt;proto>https?://)(?&lt;host>[^/?# &quot;&apos;>]+)/(?&lt;path>[^  &quot;&apos;>]+)'\n"
            "            to='${proto}${pxhost}/${pxpath}/${host}/${path}' />\n"
            "  </rule>\n"
            "  <within header=\"link\" rule=\"url\"/>\n"
            "  <within tag=\"body\" attr=\"background\" rule=\"null\"/>\n"
            "  <within tag=\"script\" attr=\"#text\" rule=\"url\"/>\n"
            "  <within tag=\"style\" attr=\"#text\" rule=\"url\"/>\n"
            "  <within attr=\"href,src\" rule=\"url\"/>\n"
            " </response>\n"
            "</filter>\n"
        ;

        std::cout << xmlconf;

        // reading and parsing XML conf
        xmlDocPtr doc = xmlParseMemory(xmlconf.c_str(), xmlconf.size());
        BOOST_CHECK(doc);
        xmlNode *root_element = xmlDocGetRootElement(doc);
        fhr.configure(root_element, true, "");
        xmlFreeDoc(doc);

        router.append(fhr);

        // create an http request
        mp::Package pack;

        mp::odr odr;
        Z_GDU *gdu_req = z_get_HTTP_Request_uri(odr,
        "http://proxyhost/proxypath/targetsite/page1.html", 0, 1);

        pack.request() = gdu_req;

        //create the http response

        const char *resp_buf =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 441\r\n"
            "Content-Type: text/html\r\n"
            "Link: <http://targetsite/file.xml>; rel=absolute\r\n"
            "Link: </dir/file.xml>; rel=relative\r\n"
            "\r\n"
            "<html><head><title>Hello proxy!</title>"
            "<style>"
            "body {"
            "  background-image:url('http://targetsite/images/bg.png');"
            "}"
            "</style>"
            "</head>"
            "<script>var jslink=\"http://targetsite/webservice.xml\";</script>"
            "<body>"
            "<p>Welcome to our website. It doesn't make it easy to get pro"
            "xified"
            "<a href=\"http://targetsite/page2.html\">"
            "  An absolute link</a>"
            "<a target=_blank href=\"http://targetsite/page3.html\">"
            "  Another abs link</a>"
            "<a href=\"/docs/page4.html\" />"
            "</body></html>";

        const char *resp_expected =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 522\r\n"
            "Content-Type: text/html\r\n"
            "Link: <http://proxyhost/proxypath/targetsite/file.xml>; rel=absolute\r\n"
            "Link: </dir/file.xml>; rel=relative\r\n"
            "\r\n"
            "<html><head><title>Hello proxy!</title>"
            "<style>"
            "body {"
            "  background-image:url('http://proxyhost/proxypath/targetsite/images/bg.png');"
            "}"
            "</style>"
            "</head>"
            "<script>var jslink=\"http://proxyhost/proxypath/targetsite/webservice.xml\";</script>"
            "<body>"
            "<p>Welcome to our website. It doesn't make it easy to get pro"
            "xified"
            "<a href=\"http://proxyhost/proxypath/targetsite/page2.html\">"
            "  An absolute link</a>"
            "<a target=\"_blank\" href=\"http://proxyhost/proxypath/targetsite/page3.html\">"
            "  Another abs link</a>"
            "<a href=\"/docs/page4.html\"/>"
            "</body></html>";

        int r;
        Z_GDU *gdu_res;
        ODR dec = odr_createmem(ODR_DECODE);
        odr_setbuf(dec, (char *) resp_buf, strlen(resp_buf), 0);
        r = z_GDU(dec, &gdu_res, 0, 0);

        BOOST_CHECK(r);
        if (r)
        {
            BOOST_CHECK_EQUAL(gdu_res->which, Z_GDU_HTTP_Response);
        }

        pack.response() = gdu_res;

        //feed to the router
        pack.router(router).move();

        //analyze the response
        Z_GDU *gdu_res_rew = pack.response().get();
        BOOST_CHECK(gdu_res_rew);
        BOOST_CHECK_EQUAL(gdu_res_rew->which, Z_GDU_HTTP_Response);

        Z_HTTP_Response *hres = gdu_res_rew->u.HTTP_Response;
        BOOST_CHECK(hres);

        //compare buffers
        std::cout << "Expected result:\n" << resp_expected << std::endl;

        ODR enc = odr_createmem(ODR_ENCODE);
        z_GDU(enc, &gdu_res_rew, 0, 0);
        char *resp_result;
        int resp_result_len;
        resp_result = odr_getbuf(enc, &resp_result_len, 0);

        BOOST_CHECK(resp_result);
        BOOST_CHECK_EQUAL((size_t) resp_result_len, strlen(resp_expected));

        std::cout << "Rewritten result:\n" << std::endl;
        fflush(stdout);
        fwrite(resp_result, 1, resp_result_len, stdout);
        fflush(stdout);
        std::cout << "\nRewritten result buf len: " << resp_result_len
            << std::endl;

        BOOST_CHECK(memcmp(resp_result, resp_expected, resp_result_len) == 0);

        odr_destroy(dec);
        odr_destroy(enc);
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

