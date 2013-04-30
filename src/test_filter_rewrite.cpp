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
#include <metaproxy/util.hpp>
#include "router_chain.hpp"
#include <metaproxy/package.hpp>

#define BOOST_REGEX_MATCH_EXTRA

#include <boost/regex.hpp>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

class FilterHeaderRewrite: public mp::filter::Base {
public:
    void process(mp::Package & package) const {
        Z_GDU *gdu = package.request().get();
        //we have an http req
        if (gdu && gdu->which == Z_GDU_HTTP_Request)
        {
          std::cout << "Request headers" << std::endl;
          Z_HTTP_Request *hreq = gdu->u.HTTP_Request;
          //dump req headers
          for (Z_HTTP_Header *header = hreq->headers;
              header != 0; 
              header = header->next) 
          {
            std::cout << header->name << ": " << header->value << std::endl;
            rewrite_req_header(header);
          }
        }
        package.move();
        gdu = package.response().get();
        if (gdu && gdu->which == Z_GDU_HTTP_Response)
        {
          std::cout << "Respose headers" << std::endl;
          Z_HTTP_Response *hr = gdu->u.HTTP_Response;
          //dump resp headers
          for (Z_HTTP_Header *header = hr->headers;
              header != 0; 
              header = header->next) 
          {
            std::cout << header->name << ": " << header->value << std::endl;
          }
        }

    };
    void configure(const xmlNode* ptr, bool test_only, const char *path) {};

    void rewrite_req_header(Z_HTTP_Header *header) const
    {
        //exec regex against value
        boost::regex e(req_uri_pat, boost::regex::perl);
        boost::smatch what;
        std::string hvalue(header->value);
        if(boost::regex_match(hvalue, what, e, boost::match_extra))
        {
            unsigned i, j;
            std::cout << "** Match found **\n   Sub-Expressions:\n";
            for(i = 0; i < what.size(); ++i)
                std::cout << "      $" << i << " = \"" << what[i] << "\"\n";
            std::cout << "   Captures:\n";
            for(i = 0; i < what.size(); ++i)
            {
                std::cout << "      $" << i << " = {";
                for(j = 0; j < what.captures(i).size(); ++j)
                {
                    if(j)
                        std::cout << ", ";
                    else
                        std::cout << " ";
                    std::cout << "\"" << what.captures(i)[j] << "\"";
                }
                std::cout << " }\n";
            }
        }
        else
        {
            std::cout << "** No Match found **\n";
        }
        //iteratate over named groups
        //set the captured values in the map
        //rewrite the header according to the hardcoded recipe
    };
    
    void configure(const std::string & req_uri_pat, 
            const std::string & resp_uri_pat) 
    {
       this->req_uri_pat = req_uri_pat;
       this->resp_uri_pat = resp_uri_pat;
    };

private:
    std::map<std::string, std::string> vars;
    std::string req_uri_pat;
    std::string resp_uri_pat;
};


BOOST_AUTO_TEST_CASE( test_filter_rewrite_1 )
{
    try
    {
       FilterHeaderRewrite fhr;
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

        FilterHeaderRewrite fhr;
        fhr.configure(".*(localhost).*", ".*(localhost).*");
        mp::filter::HTTPClient hc;
        
        router.append(fhr);
        router.append(hc);

        // create an http request
        mp::Package pack;

        mp::odr odr;
        Z_GDU *gdu_req = z_get_HTTP_Request_uri(odr, 
            "http://localhost:80/~jakub/targetsite.php", 0, 1);

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
    catch ( ... ) {
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

