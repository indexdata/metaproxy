/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

BOOST_AUTO_TEST_CASE( test_filter_rewrite_1 )
{
    try
    {
        mp::RouterChain router;
        mp::filter::HttpRewrite fhr;

        std::string xmlconf =
            "<?xml version='1.0'?>\n"
            "<filter xmlns='http://indexdata.com/metaproxy'\n"
            "        id='rewrite1' type='http_rewrite'>\n"
            " <request verbose=\"1\">\n"
            "  <rule name=\"null\"/>\n"
            "  <rule name=\"url\">\n"
            "     <rewrite from='"
    "(?&lt;proto>https?://)(?&lt;pxhost>[^ /?#]+)/(?&lt;pxpath>[^ /]+)"
    "/(?&lt;host>[^ /]+)(?&lt;path>[^ ]*)'\n"
            "            to='${proto}${host}${path}' />\n"
            "     <rewrite from='(?:Host: )(.*)'\n"
            "            to='Host: ${host}' />\n"
            "  </rule>\n"
            "  <content type=\"headers\">\n"
            "    <within header=\"link\" rule=\"null\"/>\n"
            "    <within reqline=\"1\" rule=\"url\"/>\n"
            "  </content>\n"
            " </request>\n"
            " <response verbose=\"1\">\n"
            "  <rule name=\"null\"/>\n"
            "  <rule name=\"cx\">\n"
            "     <rewrite from='^cx' to='cy'/>\n"
            "  </rule>\n"
            "  <rule name=\"url\">\n"
            "     <rewrite from='foo' to='bar'/>\n"
            "     <rewrite from='"
    "(?&lt;proto>https?://)(?&lt;host>[^/?# &quot;&apos;>]+)/(?&lt;path>[^  &quot;&apos;>]+)'\n"
            "            to='${proto}${pxhost}/${pxpath}/${host}/${path}' />\n"
            "  </rule>\n"
            "  <content type=\"headers\">\n"
            "    <within header=\"link\" rule=\"url\"/>\n"
            "  </content>\n"
            "  <content type=\"html\" mime=\"text/xml|text/html\">\n"
            "    <within tag=\"body\" attr=\"background\" rule=\"null\"/>\n"
            "    <within tag=\"script\" attr=\"#text\" type=\"quoted-literal\" rule=\"url\"/>\n"
            "    <within tag=\"style\" attr=\"#text\" rule=\"url\"/>\n"
            "    <within attr=\"href|src\" rule=\"url,cx\"/>\n"
            "    <within attr=\"onclick\" type=\"quoted-literal\" rule=\"url\"/>\n"
            "  </content>\n"
            "  <content type=\"quoted-literal\" mime=\".*javascript\">\n"
            "    <within rule=\"url\"/>\n"
            "  </content>\n"
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
        
        Z_HTTP_Request *hreq = gdu_req->u.HTTP_Request;
        z_HTTP_header_set(odr, &hreq->headers,
                          "X-Metaproxy-SkipLink", ".* skiplink.com" );
        pack.request() = gdu_req;

        //create the http response

        const char *resp_buf =
            "HTTP/1.1 200 OK\r\n"
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
            "<script>var jslink=\"http://targetsite/webservice.xml\";"
            "for (i = 0; i<foo; i++) ;\n"
            "var some=\"foo\"; foo=1;"
            "</script>"
            "<body>"
            "<p>Welcome to our website. It doesn't make it easy to get pro"
            "xified"
            "<a href=\"http://targetsite/page2.html\">"
            "  An absolute link</a>"
            "<a target=_blank href=\"http://targetsite/page3.html\">"
            "  Another abs link</a>"
            "<a href=\"/docs/page4.html\" />"
            "<A href=\"cxcx\" />"
            "<a HREF=\"cx \" onclick=\"foo(&quot;foo&quot;);\"/>\n"
            "<a href=\"http://www.skiplink.com/page5.html\">skip</a>\n"
            "</body></html>";

        const char *resp_expected =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 686\r\n"
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
            "<script>var jslink=\"http://proxyhost/proxypath/targetsite/webservice.xml\";"
            "for (i = 0; i<foo; i++) ;\n"
            "var some=\"bar\"; foo=1;"
            "</script>"
            "<body>"
            "<p>Welcome to our website. It doesn't make it easy to get pro"
            "xified"
            "<a href=\"http://proxyhost/proxypath/targetsite/page2.html\">"
            "  An absolute link</a>"
            "<a target=_blank href=\"http://proxyhost/proxypath/targetsite/page3.html\">"
            "  Another abs link</a>"
            "<a href=\"/docs/page4.html\"/>"
            "<A href=\"cycx\"/>"
            "<a HREF=\"cy \" onclick=\"foo(&quot;bar&quot;);\"/>\n"
            "<a href=\"http://www.skiplink.com/page5.html\">skip</a>\n"
            "</body></html>";

        Z_GDU *gdu_res;
        mp::odr dec(ODR_DECODE);
        mp::odr enc(ODR_ENCODE);
        odr_setbuf(dec, (char *) resp_buf, strlen(resp_buf), 0);
        int r = z_GDU(dec, &gdu_res, 0, 0);

        BOOST_CHECK(r);
        if (r)
        {
            BOOST_CHECK_EQUAL(gdu_res->which, Z_GDU_HTTP_Response);

            pack.response() = gdu_res;

            //feed to the router
            pack.router(router).move();

            //analyze the response
            Z_GDU *gdu_res_rew = pack.response().get();
            BOOST_CHECK(gdu_res_rew);
            BOOST_CHECK_EQUAL(gdu_res_rew->which, Z_GDU_HTTP_Response);

            Z_HTTP_Response *hres = gdu_res_rew->u.HTTP_Response;
            BOOST_CHECK(hres);

            z_GDU(enc, &gdu_res_rew, 0, 0);
            char *resp_result;
            int resp_result_len;
            resp_result = odr_getbuf(enc, &resp_result_len, 0);

            int equal = ((size_t) resp_result_len == strlen(resp_expected))
                && !memcmp(resp_result, resp_expected, resp_result_len);
            BOOST_CHECK(equal);

            if (!equal)
            {
                //compare buffers
                std::cout << "Expected result:\n" << resp_expected << "\n";
                std::cout << "Got result:\n";
                fflush(stdout);
                fwrite(resp_result, 1, resp_result_len, stdout);
                fflush(stdout);
                std::cout << "\nGot result buf len: " << resp_result_len
                          << "\n";
            }
        }
    }
    catch (std::exception & e) {
        std::cout << e.what();
        std::cout << std::endl;
        BOOST_CHECK (false);
    }
}

BOOST_AUTO_TEST_CASE( test_filter_rewrite_2 )
{
    try
    {
        mp::RouterChain router;
        mp::filter::HttpRewrite fhr;

        std::string xmlconf =
            "<?xml version='1.0'?>\n"
            "<filter xmlns='http://indexdata.com/metaproxy'\n"
            "        id='rewrite1' type='http_rewrite'>\n"
            " <request verbose=\"1\">\n"
            "  <rule name=\"null\"/>\n"
            "  <rule name=\"url\">\n"
            "     <rewrite from='"
    "(?&lt;proto>https?://)(?&lt;pxhost>[^ /?#]+)/(?&lt;pxpath>[^ /]+)"
    "/(?&lt;host>[^ /]+)(?&lt;path>[^ ]*)'\n"
            "            to='${proto}${host}${path}' />\n"
            "     <rewrite from='(?:Host: )(.*)'\n"
            "            to='Host: ${host}' />\n"
            "  </rule>\n"
            "  <content type=\"headers\">\n"
            "    <within header=\"link\" rule=\"null\"/>\n"
            "    <within reqline=\"1\" rule=\"url\"/>\n"
            "  </content>\n"
            " </request>\n"
            " <response verbose=\"1\">\n"
            "  <rule name=\"null\"/>\n"
            "  <rule name=\"url\">\n"
            "     <rewrite from='foo' to='bar'/>\n"
            "     <rewrite from='^cx' to='cy'/>\n"
            "     <rewrite from='"
    "(?&lt;proto>https?://)(?&lt;host>[^/?# &quot;&apos;>]+)/(?&lt;path>[^  &quot;&apos;>]+)'\n"
            "            to='${proto}${pxhost}/${pxpath}/${host}/${path}' />\n"
            "  </rule>\n"
            "  <content type=\"headers\">\n"
            "    <within header=\"link\" rule=\"url\"/>\n"
            "  </content>\n"
            "  <content type=\"quoted-literal\" mime=\".*javascript\">\n"
            "    <within rule=\"url\"/>\n"
            "  </content>\n"
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
            "Content-Type: application/javascript\r\n"
            "Link: <http://targetsite/file.xml>; rel=absolute\r\n"
            "Link: </dir/file.xml>; rel=relative\r\n"
            "\r\n"
            "// \"\n"
            "my.location = 'http://targetsite/images/bg.png';\n"
            "my.other = \"http://targetsite/images/fg.png\";\n"
            "my.thrd = \"other\";\n"
            "// \"http://targetsite/images/bg.png\n";

        const char *resp_expected =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 195\r\n"
            "Content-Type: application/javascript\r\n"
            "Link: <http://proxyhost/proxypath/targetsite/file.xml>; rel=absolute\r\n"
            "Link: </dir/file.xml>; rel=relative\r\n"
            "\r\n"
            "// \"\n"
            "my.location = 'http://proxyhost/proxypath/targetsite/images/bg.png';\n"
            "my.other = \"http://proxyhost/proxypath/targetsite/images/fg.png\";\n"
            "my.thrd = \"other\";\n"
            "// \"http://targetsite/images/bg.png\n";

        Z_GDU *gdu_res;
        mp::odr dec(ODR_DECODE);
        mp::odr enc(ODR_ENCODE);
        odr_setbuf(dec, (char *) resp_buf, strlen(resp_buf), 0);
        int r = z_GDU(dec, &gdu_res, 0, 0);

        BOOST_CHECK(r);
        if (r)
        {
            BOOST_CHECK_EQUAL(gdu_res->which, Z_GDU_HTTP_Response);

            pack.response() = gdu_res;

            //feed to the router
            pack.router(router).move();

            //analyze the response
            Z_GDU *gdu_res_rew = pack.response().get();
            BOOST_CHECK(gdu_res_rew);
            BOOST_CHECK_EQUAL(gdu_res_rew->which, Z_GDU_HTTP_Response);

            Z_HTTP_Response *hres = gdu_res_rew->u.HTTP_Response;
            BOOST_CHECK(hres);

            z_GDU(enc, &gdu_res_rew, 0, 0);
            char *resp_result;
            int resp_result_len;
            resp_result = odr_getbuf(enc, &resp_result_len, 0);

            int equal = ((size_t) resp_result_len == strlen(resp_expected))
                && !memcmp(resp_result, resp_expected, resp_result_len);
            BOOST_CHECK(equal);

            if (!equal)
            {
                //compare buffers
                std::cout << "Expected result:\n" << resp_expected << "\n";
                std::cout << "Got result:\n";
                fflush(stdout);
                fwrite(resp_result, 1, resp_result_len, stdout);
                fflush(stdout);
                std::cout << "\nGot result buf len: " << resp_result_len
                          << "\n";
            }
        }
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

