
#include <iostream>
#include "design.h"

class TFilter: public yp2::Filter {
public:
    yp2::Package & process(yp2::Package & package) const {
	return package;
    };
};
    
int main(int argc, char **argv)
{
    // test filter set/get/exception
    try {
        TFilter filter;
	
        filter.name("filter1");
        std::cout <<  filter.name() << std::endl;

	if (filter.name() != "filter1")
	{
	    std::cout << "filter name does not match 1\n";
	    exit(1);
	}

        filter.name() = "filter1 rename";
        std::cout <<  filter.name() << std::endl;
	if (filter.name() != "filter1 rename")
	{
	    std::cout << "filter name does not match 2\n";
	    exit(1);
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
