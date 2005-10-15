/* $Id: filter_log.cpp,v 1.3 2005-10-15 14:09:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */


#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include "filter_log.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <iostream>

yp2::filter::Log::Log() {}

void yp2::filter::Log::process(Package &package) const {

    Z_GDU *gdu;

    gdu = package.request().get();
    if (gdu)
    {
	ODR odr = odr_createmem(ODR_PRINT);
	z_GDU(odr, &gdu, 0, 0);
	odr_destroy(odr);
    }
    package.move();

    gdu = package.response().get();
    if (gdu)
    {
	ODR odr = odr_createmem(ODR_PRINT);
	z_GDU(odr, &gdu, 0, 0);
	odr_destroy(odr);
    }
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
