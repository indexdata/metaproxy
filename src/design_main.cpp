
#include <iostream>
#include  "design.h"

int main(int argc, char **argv) {

    // test filter set/get/exception
    try {
        std::cout << "\nTRY" << "\n";
        yp2::Filter filter;
    
        filter.name("filter1");
        std::cout << "filter: " << filter.name() << "\n";

        filter.name() = "filter1 rename";
        std::cout << "filter: " << filter.name() << "\n";

        throw yp2::Filter_Exception("finished");
    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
    }

  
    try {
        std::cout << "\nTRY" << "\n";

        yp2::Filter filter1;
        filter1.name("filter1");
    
        yp2::Filter filter2;
        filter2.name() = "filter2";

        std::cout << "filter1 filter2" << "\n";
    
        yp2::Router router1;
        router1.rule(filter1);
        std::cout << "router1.rule(filter1)" << "\n";

        yp2::Router router2;
        router2.rule(filter2);
        std::cout << "router2.rule(filter2)" << "\n";

        yp2::Package pack_in;
        pack_in.data(7).router(router1);
        std::cout << "pack_in.data(7).router(router1)" << "\n";

        pack_in.move();
        std::cout << "pack_in.move()" << "\n";

        pack_in.router(router2);
        std::cout << "pack_in.router(router2)" << "\n";

        pack_in.move();
        std::cout << "pack_in.move()" << "\n";

        throw  yp2::Router_Exception("finished");

    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
