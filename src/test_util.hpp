/* $Id: test_util.hpp,v 1.1 2005-10-25 21:32:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef TEST_UTIL_HPP
#define TEST_UTIL_HPP

#include <yaz/z-core.h>
#include <string>
namespace yp2 {
    struct util  {
	static bool pqf(ODR odr, Z_APDU *apdu, const std::string &q);
    };
}
#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
