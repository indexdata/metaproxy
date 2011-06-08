/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef YP2_UTIL_HPP
#define YP2_UTIL_HPP

#include "package.hpp"

#include <yaz/z-core.h>

#include <boost/utility.hpp>
#include <boost/scoped_ptr.hpp>

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

        const char * 
        record_composition_to_esn(Z_RecordComposition *comp);

        std::string http_header_value(const Z_HTTP_Header* header, 
                                               const std::string name);

        std::string http_headers_debug(const Z_HTTP_Request &http_req);        

        void http_response(metaproxy_1::Package &package, 
                           const std::string &content, 
                           int http_code = 200);


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
                                 const std::string vhost, 
                                 const int cat);

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

        void piggyback_sr(Z_SearchRequest *sreq,
                          Odr_int result_set_size,
                          Odr_int &number_to_present,
                          const char **element_set_name);

        void piggyback(Odr_int smallSetUpperBound,
                       Odr_int largeSetLowerBound,
                       Odr_int mediumSetPresentNumber,
                       const char *smallSetElementSetNames,
                       const char *mediumSetElementSetNames,
                       Odr_int result_set_size,
                       Odr_int &number_to_present,
                       const char **element_set_name);

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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

