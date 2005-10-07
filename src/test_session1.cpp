
#include <iostream>

//#include "filter.hpp"
//#include "router.hpp"
//#include "package.hpp"
#include "session.hpp"

  
int main(int argc, char **argv)
{
    // test session 
    try {
        yp2::Session session;
        unsigned long int id;
        id = session.id();
        std::cout <<  "Session.id() == " << id << std::endl;
        id = session.id();
        std::cout <<  "Session.id() == " << id << std::endl;
        id = session.id();
        std::cout <<  "Session.id() == " << id << std::endl;

	if (id != 3)
	{
	    std::cout << "Fail: Session.id() != 3\n";
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
