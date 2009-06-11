/* This file is part of Metaproxy.
   Copyright (C) 2005-2009 Index Data

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

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <yaz/options.h>
#include "util.hpp"
#include "filter_frontend_net.hpp"
#include "filter_z3950_client.hpp"
#include "filter_virt_db.hpp"
#include "filter_session_shared.hpp"
#include "filter_log.hpp"

#include "router_chain.hpp"
#include "session.hpp"
#include "package.hpp"

namespace mp = metaproxy_1;

class HTTPFilter: public mp::filter::Base {
public:
    void process(mp::Package & package) const {
        if (package.session().is_closed())
        {
            // std::cout << "Got Close.\n";
        }
        
        Z_GDU *gdu = package.request().get();
        if (gdu && gdu->which == Z_GDU_HTTP_Request)
        {
            mp::odr odr;
            Z_GDU *gdu = z_get_HTTP_Response(odr, 200);
            Z_HTTP_Response *http_res = gdu->u.HTTP_Response;
            
            z_HTTP_header_add(odr, &http_res->headers,
                              "Content-Type", "text/plain");
            
            http_res->content_buf = 
                odr_strdup(odr, "Welcome to Metaproxy");
            http_res->content_len = strlen(http_res->content_buf);
            
            package.response() = gdu;
        }
        return package.move();
    };
};

int main(int argc, char **argv)
{
    try 
    {
        std::vector<std::string> ports;
        int duration = -1;
        int ret;
        char *arg;

        while ((ret = options("h{help}d{duration}:p{port}:", 
                              argv, argc, &arg)) != -2)
        {
            switch(ret)
            {
            case -1:
                std::cerr << "bad option " << arg << std::endl;
            case 'h':
                std::cerr << "ex_filter_frontend_net\n"
                    " -h|--help       help\n"
                    " -d|--duration n duration\n"
                    " -p|--port n     port number\n"
                          << std::endl;
                break;
            case 'p':
                ports.push_back(arg);
                break;
            case 'd':
                duration = atoi(arg);
                break;
            }
        }
        {
            for (size_t i = 0; i<ports.size(); i++)
                std::cout << "port " << i << " " << ports[i] << "\n";

	    mp::RouterChain router;

            // put frontend filter in router
            mp::filter::FrontendNet filter_front;
            filter_front.set_ports(ports);

            // 0=no time, >0 timeout in seconds
            if (duration != -1)
                filter_front.set_listen_duration(duration);

	    router.append(filter_front);

            // put log filter in router
            mp::filter::Log filter_log_front("FRONT");
            router.append(filter_log_front);

            // put Virt db filter in router
            mp::filter::VirtualDB filter_virt_db;
            filter_virt_db.add_map_db2target("gils", "indexdata.dk/gils",
                                            "");
            filter_virt_db.add_map_db2target("Default", "localhost:9999/Default",
                                            "");
            filter_virt_db.add_map_db2target("2", "localhost:9999/Slow", "");
	    router.append(filter_virt_db);

            mp::filter::SessionShared filter_session_shared;
            //router.append(filter_session_shared);

            mp::filter::Log filter_log_back("BACK");
            router.append(filter_log_back);

            // put HTTP backend filter in router
            HTTPFilter filter_init;
	    router.append(filter_init);

            // put Z39.50 backend filter in router
            mp::filter::Z3950Client z3950_client;
	    router.append(z3950_client);

            mp::Session session;
            mp::Origin origin;
	    mp::Package pack(session, origin);
	    
	    pack.router(router).move(); 
        }
    }
    catch ( ... ) {
        std::cerr << "unknown exception\n";
        std::exit(1);
    }
    std::exit(0);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

