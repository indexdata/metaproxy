/* $Id: util.hpp,v 1.9 2006-01-18 10:57:27 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef YP2_UTIL_HPP
#define YP2_UTIL_HPP

#include <yaz/z-core.h>
#include <string>
#include <list>

#include <boost/utility.hpp>

namespace yp2 {
    namespace util  {
	bool pqf(ODR odr, Z_APDU *apdu, const std::string &q);
        Z_ReferenceId **get_referenceId(Z_APDU *apdu);
        Z_APDU *create_APDU(ODR odr, int type, Z_APDU *in_apdu);
        bool set_databases_from_zurl(ODR odr, std::string zurl,
                                     int *db_num, char ***db_strings);
        void split_zurl(std::string zurl, std::string &host,
                        std::list<std::string> &db);

        int get_vhost_otherinfo(Z_OtherInformation **otherInformation,
                                bool remove_flag,
                                std::list<std::string> &vhosts);
        void set_vhost_otherinfo(Z_OtherInformation **otherInformation,
                                 ODR odr,
                                 const std::list<std::string> &vhosts);
        void get_init_diagnostics(Z_InitResponse *res,
                                  int &error_code, std::string &addinfo);
        void get_default_diag(Z_DefaultDiagFormat *r,
                              int &error_code, std::string &addinfo);

        void piggyback(int smallSetUpperBound,
                       int largeSetLowerBound,
                       int mediumSetPresentNumber,
                       int result_set_size,
                       int &number_to_present);
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
