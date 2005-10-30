/* $Id: util.cpp,v 1.3 2005-10-30 18:51:21 adam Exp $
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

Z_APDU *yp2::odr::create_close(int reason, const char *addinfo)
{
    Z_APDU *apdu = zget_APDU(m_odr, Z_APDU_close);
    
    *apdu->u.close->closeReason = reason;
    if (addinfo)
        apdu->u.close->diagnosticInformation = odr_strdup(m_odr, addinfo);
    return apdu;
}

Z_APDU *yp2::odr::create_initResponse(int error, const char *addinfo)
{
    Z_APDU *apdu = zget_APDU(m_odr, Z_APDU_initResponse);
    if (error)
    {
        apdu->u.initResponse->userInformationField =
            zget_init_diagnostics(m_odr, error, addinfo);
        *apdu->u.initResponse->result = 0;
    }
    return apdu;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
