
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "config.hpp"

#include "filter_frontend_net.hpp"

#include "router.hpp"
#include "session.hpp"
#include "package.hpp"

class FilterInit: public yp2::Filter {
public:
    void process(yp2::Package & package) const {

        if (package.session().is_closed())
        {
            // std::cout << "Got Close.\n";
        }
       
        Z_GDU *gdu = package.request().get();
        if (gdu)
        {
            ODR odr = odr_createmem(ODR_ENCODE);
            switch(gdu->which)
            {
            case Z_GDU_Z3950:
                // std::cout << "Got PDU. Sending init response\n";
                Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
                
                apdu->u.initResponse->implementationName = "YP2/YAZ";
                
                package.response() = apdu;
                break;
            case Z_GDU_HTTP_Request:
                Z_GDU *gdu = z_get_HTTP_Response(odr, 200);
                Z_HTTP_Response *http_res = gdu->u.HTTP_Response;
        
                z_HTTP_header_add(odr, &http_res->headers,
                                  "Content-Type", "text/plain");
           
                http_res->content_buf = 
                    odr_strdup(odr, "Welcome to YP2");
                http_res->content_len = strlen(http_res->content_buf);

                package.response() = gdu;
                break;
            default:
                break;
            } 
            odr_destroy(odr);
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
            ;

        po::variables_map vm;        
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);    

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        {
	    yp2::RouterChain router;

            // put in frontend first
            yp2::FilterFrontendNet filter_front;
            filter_front.listen_address() = "tcp:@:9999";

            // 0=no time, >0 timeout in seconds
            if (vm.count("duration")) {
                filter_front.listen_duration() = 
                    vm["duration"].as<int>();
            }
	    router.rule(filter_front);

            // put in a backend
            FilterInit filter_init;
	    router.rule(filter_init);

            yp2::Session session;
            yp2::Origin origin;
	    yp2::Package pack(session, origin);
	    
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
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
