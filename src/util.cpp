/* $Id: util.cpp,v 1.1 2005-10-26 18:53:49 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include <yaz/pquery.h>
#include "util.hpp"

bool yp2::util::pqf(ODR odr, Z_APDU *apdu, const std::string &q) {
    YAZ_PQF_Parser pqf_parser = yaz_pqf_create();
    
    Z_RPNQuery *rpn = yaz_pqf_parse(pqf_parser, odr, q.c_str());
    if (!rpn)
    {
	yaz_pqf_destroy(pqf_parser);
	return false;
    }
    yaz_pqf_destroy(pqf_parser);
    Z_Query *query = (Z_Query *) odr_malloc(odr, sizeof(Z_Query));
    query->which = Z_Query_type_1;
    query->u.type_1 = rpn;
    
    apdu->u.searchRequest->query = query;
    return true;
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
