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

#ifndef FILTER_HTTP_REWRITE_HPP
#define FILTER_HTTP_REWRITE_HPP

#include <metaproxy/filter.hpp>
#include <vector>
#include <map>
#include <metaproxy/util.hpp>

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class HttpRewrite : public Base {
        public:
            typedef std::pair<std::string, std::string> string_pair;
            typedef std::vector<string_pair> spair_vec;
            typedef spair_vec::iterator spv_iter;
            HttpRewrite();
            ~HttpRewrite();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
            void configure(const spair_vec req_uri_pats,
                           const spair_vec res_uri_pats); 
        private:
            spair_vec req_uri_pats;
            spair_vec res_uri_pats;
            std::vector<std::map<int, std::string> > req_groups_bynum;
            std::vector<std::map<int, std::string> > res_groups_bynum;
            void rewrite_reqline (mp::odr & o, Z_HTTP_Request *hreq,
                    std::map<std::string, std::string> & vars) const;
            void rewrite_headers (mp::odr & o, Z_HTTP_Header *headers,
                    std::map<std::string, std::string> & vars) const; 
            void rewrite_body (mp::odr & o, char **content_buf, int *content_len,
                    std::map<std::string, std::string> & vars) const;
            const std::string test_patterns(
                    std::map<std::string, std::string> & vars,
                    const std::string & txt, 
                    const spair_vec & uri_pats,
                    const std::vector<std::map<int, std::string> > & groups_bynum_vec) const;
            const std::string search_replace(
                    std::map<std::string, std::string> & vars,
                    const std::string & txt,
                    const std::string & uri_re,
                    const std::string & uri_pat,
                    const std::map<int, std::string> & groups_bynum) const;
            static void parse_groups(
                    const spair_vec & uri_pats,
                    std::vector<std::map<int, std::string> > & groups_bynum_vec);
            static std::string sub_vars (const std::string & in, 
                    const std::map<std::string, std::string> & vars);
        };
    }
}

extern "C" {
    extern struct metaproxy_1_filter_struct metaproxy_1_filter_http_rewrite;
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

