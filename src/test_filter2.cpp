
#include <iostream>
#include "design.h"

class FilterConstant: public yp2::Filter {
public:
    yp2::Package & process(yp2::Package & package) const {
        std::cout << name() + ".process()" << std::endl;
	package.data() = 1234;
	return package.move();
    };
};


class FilterDouble: public yp2::Filter {
public:
    yp2::Package & process(yp2::Package & package) const {
        std::cout <<  name() + ".process()" << std::endl;
	package.data() = package.data() * 2;
	return package.move();
    };
};
    
    
int main(int argc, char **argv)
{
    try {
	FilterConstant fc;
        fc.name() = "FilterConstant";
	FilterDouble fd;
        fd.name() = "FilterDouble";

	{
	    yp2::RouterChain router1;
	    
	    // test filter set/get/exception
	    router1.rule(fc);
	    
	    router1.rule(fd);
	    
	    yp2::Package pack_in;
	    
	    yp2::Package pack_out;
	    
	    pack_out = pack_in.router(router1).move(); 
	    
	    if (pack_out.data() != 2468)
	    {
		exit(1);
	    }
	}
	{
	    yp2::RouterChain router1;
	    
	    router1.rule(fd);
	    router1.rule(fc);
	    
	    yp2::Package pack_in;
	    
	    yp2::Package pack_out;
	    
	    pack_out = pack_in.router(router1).move(); 
	    
	    if (pack_out.data() != 1234)
	    {
		exit(1);
	    }
	}

    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
	exit(1);
    }
    exit(0);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
