/* $Id: ex_filter_frontend_net.cpp,v 1.29 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>
namespace po = boost::program_options;


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
                odr_strdup(odr, "Welcome to YP2");
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
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("duration", po::value<int>(),
             "number of seconds for server to exist")
            ("port", po::value< std::vector<std::string> >(), "listener port")
            ;

        po::positional_options_description p;
        p.add("port", -1);

        po::variables_map vm;        
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);    

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (vm.count("port"))
        {
            std::vector<std::string> ports = 
                vm["port"].as< std::vector<std::string> >();

            for (size_t i = 0; i<ports.size(); i++)
                std::cout << "port " << i << " " << ports[i] << "\n";

	    mp::RouterChain router;

            // put frontend filter in router
            mp::filter::FrontendNet filter_front;
            filter_front.ports() = ports;

            // 0=no time, >0 timeout in seconds
            if (vm.count("duration")) {
                filter_front.listen_duration() = vm["duration"].as<int>();
            }
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
