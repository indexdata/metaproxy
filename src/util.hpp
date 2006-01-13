/* $Id: util.hpp,v 1.4 2006-01-13 15:09:35 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef YP2_UTIL_HPP
#define YP2_UTIL_HPP

#include <yaz/z-core.h>
#include <string>

#include <boost/utility.hpp>

namespace yp2 {
    namespace util  {
	bool pqf(ODR odr, Z_APDU *apdu, const std::string &q);
        Z_ReferenceId **get_referenceId(Z_APDU *apdu);
        Z_APDU *create_APDU(ODR odr, int type, Z_APDU *in_apdu);
    };

    class odr : public boost::noncopyable {
    public:
        odr(int type);
        odr();
        ~odr();
        operator ODR() const;
        Z_APDU *create_close(Z_APDU *in_apdu, 
                             int reason, const char *addinfo);
        Z_APDU *create_initResponse(Z_APDU *in_apdu, 
                                    int error, const char *addinfo);
        Z_APDU *create_searchResponse(Z_APDU *in_apdu,
                                      int error, const char *addinfo);
        Z_APDU *create_presentResponse(Z_APDU *in_apdu,
                                       int error, const char *addinfo);
        Z_APDU *create_scanResponse(Z_APDU *in_apdu,
                                    int error, const char *addinfo);
        Z_APDU *create_APDU(int type, Z_APDU *in_apdu);
    private:
        ODR m_odr;
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
