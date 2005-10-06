
#include <iostream>
#include  "p3_filter.h"



int main(int argc, char **argv) {

   // test filter set/get/exception
  try {
    std::cout << "\nTRY" << "\n";
    p3::Filter filter;
    unsigned int tmp;
    
    filter.max_front_workers(1).max_front_workers(2);
    tmp = filter.max_front_workers();
    std::cout << "workers: " << tmp << "\n";

    filter.max_front_workers() = 3;
    tmp = filter.max_front_workers();
    std::cout << "workers: " << tmp << "\n";

    throw p3::Filter_Exception("finished");
  }
  catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }

  
  try {
    std::cout << "\nTRY" << "\n";

    p3::Filter filter1;
    p3::Filter filter2;

    std::cout << "filter1 filter2" << "\n";
    
    p3::Router router1;
    router1.rule(filter1);
    std::cout << "router1.rule(filter1)" << "\n";

    p3::Router router2;
    router2.rule(filter2);
    std::cout << "router2.rule(filter2)" << "\n";

    p3::Package pack_in;
    pack_in.data(7).router(router1);
    std::cout << "pack_in.data(7).router(router1)" << "\n";

    pack_in.move();
    std::cout << "pack_in.move()" << "\n";

    pack_in.router(router2);
    std::cout << "pack_in.router(router2)" << "\n";

    pack_in.move();
    std::cout << "pack_in.move()" << "\n";

    throw  p3::Router_Exception("finished");

  }
  catch (std::exception &e) {
    std::cout << e.what() << "\n";
  }



}

