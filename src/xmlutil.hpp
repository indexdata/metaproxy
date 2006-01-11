/* $Id: xmlutil.hpp,v 1.3 2006-01-11 13:13:49 adam Exp $
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
        bool check_element_yp2(const xmlNode *ptr, 
                               const std::string &name);
        std::string get_route(const xmlNode *node);

        const xmlNode* jump_to(const xmlNode* node, int node_type);

        const xmlNode* jump_to_next(const xmlNode* node, int node_type);
        
        const xmlNode* jump_to_children(const xmlNode* node, int node_type);
    }
    class XMLError : public std::runtime_error {
    public:
        XMLError(const std::string msg) :
            std::runtime_error("XMLError : " + msg) {} ;
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
