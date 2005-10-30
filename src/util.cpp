/* $Id: util.cpp,v 1.2 2005-10-30 17:13:36 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include <yaz/odr.h>
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

yp2::odr::odr(int type)
{
    m_odr = odr_createmem(type);
}

yp2::odr::odr()
{
    m_odr = odr_createmem(ODR_ENCODE);
}

yp2::odr::~odr()
{
    odr_destroy(m_odr);
}

yp2::odr::operator ODR() const
{
    return m_odr;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
