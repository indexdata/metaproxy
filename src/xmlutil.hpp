/* $Id: xmlutil.hpp,v 1.1 2006-01-09 13:43:59 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef XML_UTIL_HPP
#define XML_UTIL_HPP

#include <string>
#include <stdexcept>
#include <libxml/tree.h>

namespace yp2 {
    namespace xml {
        std::string get_text(const xmlNode *ptr);
        bool is_element(const xmlNode *ptr, 
                        const std::string &ns,
                        const std::string &name);
        bool is_element_yp2(const xmlNode *ptr, const std::string &name);
    }
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
