/* $Id: filter_http_file.hpp,v 1.3 2006-02-02 11:33:46 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_HTTP_FILE_HPP
#define FILTER_HTTP_FILE_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class HttpFile : public Base {
            class Rep;
            struct Area;
            class Mime;
            boost::scoped_ptr<Rep> m_p;
        public:
            HttpFile();
            ~HttpFile();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_http_file;
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
