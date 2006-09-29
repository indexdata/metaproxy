/* $Id: util.hpp,v 1.19 2006-09-29 08:42:47 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_UTIL_HPP
#define YP2_UTIL_HPP

#include "package.hpp"

#include <yaz/z-core.h>

#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <string>


namespace metaproxy_1 {
    namespace util  {

        
        template<typename T> 
        std::string to_string(const T& t)
        {
            std::ostringstream o;
            if(o << t)
                return o.str();    
            return std::string();
        };

        std::string http_header_value(const Z_HTTP_Header* header, 
                                               const std::string name);

        int memcmp2(const void *buf1, int len1, const void *buf2, int len2);

        std::string database_name_normalize(const std::string &s);

	bool pqf(ODR odr, Z_APDU *apdu, const std::string &q);

        std::string zQueryToString(Z_Query *query);

        Z_ReferenceId **get_referenceId(const Z_APDU *apdu);

        void transfer_referenceId(ODR odr, const Z_APDU *src, Z_APDU *dst);

        Z_APDU *create_APDU(ODR odr, int type, const Z_APDU *in_apdu);

        bool set_databases_from_zurl(ODR odr, std::string zurl,
                                     int *db_num, char ***db_strings);

        void split_zurl(std::string zurl, std::string &host,
                        std::list<std::string> &db);
        
        void get_vhost_otherinfo(Z_OtherInformation *otherInformation,
                                 std::list<std::string> &vhosts);
        
        int remove_vhost_otherinfo(Z_OtherInformation **otherInformation,
                                   std::list<std::string> &vhosts);

        void set_vhost_otherinfo(Z_OtherInformation **otherInformation,
                                 ODR odr,
                                 const std::list<std::string> &vhosts);

        int get_or_remove_vhost_otherinfo(
            Z_OtherInformation **otherInformation,
            bool remove_flag,
            std::list<std::string> &vhosts);

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
        Z_APDU *create_close(const Z_APDU *in_apdu, 
                             int reason, const char *addinfo);
        Z_APDU *create_initResponse(const Z_APDU *in_apdu, 
                                    int error, const char *addinfo);
        Z_APDU *create_searchResponse(const Z_APDU *in_apdu,
                                      int error, const char *addinfo);
        Z_APDU *create_presentResponse(const Z_APDU *in_apdu,
                                       int error, const char *addinfo);
        Z_APDU *create_scanResponse(const Z_APDU *in_apdu,
                                    int error, const char *addinfo);
        Z_APDU *create_APDU(int type, const Z_APDU *in_apdu);

        Z_GDU *create_HTTP_Response(metaproxy_1::Session &session,
                                    Z_HTTP_Request *req, int code);
    private:
        ODR m_odr;
    };

    class PlainFile {
        class Rep;
        boost::scoped_ptr<Rep> m_p;
    public:
        PlainFile();
        ~PlainFile();
        bool open(const std::string &fname);
        bool getline(std::vector<std::string> &args);
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
