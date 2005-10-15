

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include "filter_log.hpp"

#include <yaz/zgdu.h>
#include <yaz/log.h>

#include <iostream>

yp2::FilterLog::FilterLog() {}

void yp2::FilterLog::process(Package &package) const {

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


