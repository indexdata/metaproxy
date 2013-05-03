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

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

typedef std::pair<std::string, std::string> string_pair;
typedef std::vector<string_pair> spair_vec;
typedef spair_vec::iterator spv_iter;

class FilterHeaderRewrite: public mp::filter::Base {
public:
    void process(mp::Package & package) const {
        Z_GDU *gdu = package.request().get();
        //map of request/response vars
        std::map<std::string, std::string> vars;
        //we have an http req
        if (gdu && gdu->which == Z_GDU_HTTP_Request)
        {
            Z_HTTP_Request *hreq = gdu->u.HTTP_Request;
            mp::odr o;
            //rewrite the request line
            std::string path;
            if (strstr(hreq->path, "http://") == hreq->path)
            {
                std::cout << "Path in the method line is absolute, " 
                    "possibly a proxy request\n";
                path += hreq->path;
            }
            else
            {
                //TODO what about proto
               path += z_HTTP_header_lookup(hreq->headers, "Host");
               path += hreq->path; 
            }
            std::cout << "Proxy request URL is " << path << std::endl;
            std::string npath = 
                test_patterns(vars, path, req_uri_pats, req_groups_bynum);
            std::cout << "Resp request URL is " << npath << std::endl;
            if (!npath.empty())
                hreq->path = odr_strdup(o, npath.c_str());
            std::cout << ">> Request headers" << std::endl;
            //iterate headers
            for (Z_HTTP_Header *header = hreq->headers;
                    header != 0; 
                    header = header->next) 
            {
                std::cout << header->name << ": " << header->value << std::endl;
                std::string out = test_patterns(vars, 
                        std::string(header->value), 
                        req_uri_pats, req_groups_bynum);
                if (!out.empty())
                    header->value = odr_strdup(o, out.c_str());
            }
            package.request() = gdu;
        }
        package.move();
        gdu = package.response().get();
        if (gdu && gdu->which == Z_GDU_HTTP_Response)
        {
            Z_HTTP_Response *hr = gdu->u.HTTP_Response;
            std::cout << "Response " << hr->code;
            std::cout << "<< Respose headers" << std::endl;
            mp::odr o;
            //iterate headers
            for (Z_HTTP_Header *header = hr->headers;
                    header != 0; 
                    header = header->next) 
            {
                std::cout << header->name << ": " << header->value << std::endl;
                std::string out = test_patterns(vars,
                        std::string(header->value), 
                        res_uri_pats, res_groups_bynum); 
                if (!out.empty())
                    header->value = odr_strdup(o, out.c_str());
            }
            package.response() = gdu;
        }
    };

    void configure(const xmlNode* ptr, bool test_only, const char *path) {};

    /**
     * Tests pattern from the vector in order and executes recipe on
       the first match.
     */
    const std::string test_patterns(
            std::map<std::string, std::string> & vars,
            const std::string & txt, 
            const spair_vec & uri_pats,
            const std::vector<std::map<int, std::string> > & groups_bynum_vec)
        const
    {
        for (int i = 0; i < uri_pats.size(); i++) 
        {
            std::string out = search_replace(vars, txt, 
                    uri_pats[i].first, uri_pats[i].second,
                    groups_bynum_vec[i]);
            if (!out.empty()) return out;
        }
        return "";
    }


    const std::string search_replace(
            std::map<std::string, std::string> & vars,
            const std::string & txt,
            const std::string & uri_re,
            const std::string & uri_pat,
            const std::map<int, std::string> & groups_bynum) const
    {
        //exec regex against value
        boost::regex re(uri_re);
        boost::smatch what;
        std::string::const_iterator start, end;
        start = txt.begin();
        end = txt.end();
        std::string out;
        while (regex_search(start, end, what, re)) //find next full match
        {
            unsigned i;
            for (i = 1; i < what.size(); ++i)
            {
                //check if the group is named
                std::map<int, std::string>::const_iterator it
                    = groups_bynum.find(i);
                if (it != groups_bynum.end()) 
                {   //it is
                    std::string name = it->second;
                    if (!what[i].str().empty())
                        vars[name] = what[i];
                }

            }
            //prepare replacement string
            std::string rvalue = sub_vars(uri_pat, vars);
            //rewrite value
            std::string rhvalue = what.prefix().str() 
                + rvalue + what.suffix().str();
            std::cout << "! Rewritten '"+what.str(0)+"' to '"+rvalue+"'\n";
            out += rhvalue;
            start = what[0].second; //move search forward
        }
        return out;
    }

    static void parse_groups(
            const spair_vec & uri_pats,
            std::vector<std::map<int, std::string> > & groups_bynum_vec)
    {
        for (int h = 0; h < uri_pats.size(); h++) 
        {
            int gnum = 0;
            bool esc = false;
            //regex is first, subpat is second
            std::string str = uri_pats[h].first;
            //for each pair we have an indexing map
            std::map<int, std::string> groups_bynum;
            for (int i = 0; i < str.size(); ++i)
            {
                if (!esc && str[i] == '\\')
                {
                    esc = true;
                    continue;
                }
                if (!esc && str[i] == '(') //group starts
                {
                    gnum++;
                    if (i+1 < str.size() && str[i+1] == '?') //group with attrs 
                    {
                        i++;
                        if (i+1 < str.size() && str[i+1] == ':') //non-capturing
                        {
                            if (gnum > 0) gnum--;
                            i++;
                            continue;
                        }
                        if (i+1 < str.size() && str[i+1] == 'P') //optional, python
                            i++;
                        if (i+1 < str.size() && str[i+1] == '<') //named
                        {
                            i++;
                            std::string gname;
                            bool term = false;
                            while (++i < str.size())
                            {
                                if (str[i] == '>') { term = true; break; }
                                if (!isalnum(str[i])) 
                                    throw mp::filter::FilterException
                                        ("Only alphanumeric chars allowed, found "
                                         " in '" 
                                         + str 
                                         + "' at " 
                                         + boost::lexical_cast<std::string>(i)); 
                                gname += str[i];
                            }
                            if (!term)
                                throw mp::filter::FilterException
                                    ("Unterminated group name '" + gname 
                                     + " in '" + str +"'");
                            groups_bynum[gnum] = gname;
                            std::cout << "Found named group '" << gname 
                                << "' at $" << gnum << std::endl;
                        }
                    }
                }
                esc = false;
            }
            groups_bynum_vec.push_back(groups_bynum);
        }
    }

    static std::string sub_vars (const std::string & in, 
            const std::map<std::string, std::string> & vars)
    {
        std::string out;
        bool esc = false;
        for (int i = 0; i < in.size(); ++i)
        {
            if (!esc && in[i] == '\\')
            {
                esc = true;
                continue;
            }
            if (!esc && in[i] == '$') //var
            {
                if (i+1 < in.size() && in[i+1] == '{') //ref prefix
                {
                    ++i;
                    std::string name;
                    bool term = false;
                    while (++i < in.size()) 
                    {
                        if (in[i] == '}') { term = true; break; }
                        name += in[i];
                    }
                    if (!term) throw mp::filter::FilterException
                        ("Unterminated var ref in '"+in+"' at "
                         + boost::lexical_cast<std::string>(i));
                    std::map<std::string, std::string>::const_iterator it
                        = vars.find(name);
                    if (it != vars.end())
                    {
                        out += it->second;
                    }
                }
                else
                {
                    throw mp::filter::FilterException
                        ("Malformed or trimmed var ref in '"
                         +in+"' at "+boost::lexical_cast<std::string>(i)); 
                }
                continue;
            }
            //passthru
            out += in[i];
            esc = false;
        }
        return out;
    }
    
    void configure(
            const spair_vec req_uri_pats,
            const spair_vec res_uri_pats)
    {
       //TODO should we really copy them out?
       this->req_uri_pats = req_uri_pats;
       this->res_uri_pats = res_uri_pats;
       //pick up names
       parse_groups(req_uri_pats, req_groups_bynum);
       parse_groups(res_uri_pats, res_groups_bynum);
    };

private:
    std::map<std::string, std::string> vars;
    spair_vec req_uri_pats;
    spair_vec res_uri_pats;
    std::vector<std::map<int, std::string> > req_groups_bynum;
    std::vector<std::map<int, std::string> > res_groups_bynum;

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
        
        spair_vec vec_req;
        vec_req.push_back(std::make_pair(
        "(?<proto>http\\:\\/\\/s?)(?<pxhost>[^\\/?#]+)\\/(?<pxpath>[^\\/]+)"
        "\\/(?<target>.+)",
        "${proto}${target}"
        ));
        vec_req.push_back(std::make_pair(
        "proxyhost",
        "localhost"
        ));

        spair_vec vec_res;
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

