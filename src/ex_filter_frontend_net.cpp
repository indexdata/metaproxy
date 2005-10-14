
#include <cstdlib>
#include <iostream>
#include <stdexcept>

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
            // std::cout << "Got PDU. Sending init response\n";
            ODR odr = odr_createmem(ODR_ENCODE);
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_initResponse);
            
            apdu->u.initResponse->implementationName = "YP2/YAZ";
            
            package.response() = apdu;
            odr_destroy(odr);
        }
        return package.move();
    };
};

int main(int argc, char **argv)
{
    try 
    {
        {
	    yp2::RouterChain router;

            // put in frontend first
            yp2::FilterFrontendNet filter_front;
            filter_front.listen_address() = "tcp:@:9999";
            //filter_front.listen_duration() = 1;  // listen a short time only
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
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
