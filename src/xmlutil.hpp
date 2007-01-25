/* $Id: xmlutil.hpp,v 1.11 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

#ifndef XML_UTIL_HPP
#define XML_UTIL_HPP

#include <string>
#include <stdexcept>
#include <libxml/tree.h>

namespace metaproxy_1 {
    namespace xml {
        std::string get_text(const struct _xmlAttr *ptr);
        std::string get_text(const xmlNode *ptr);
        bool get_bool(const xmlNode *ptr, bool default_value);
        int get_int(const xmlNode *ptr, int default_value);
        bool check_attribute(const _xmlAttr *ptr, 
                        const std::string &ns,
                        const std::string &name);
        bool is_attribute(const _xmlAttr *ptr, 
                        const std::string &ns,
                        const std::string &name);
        bool is_element(const xmlNode *ptr, 
                        const std::string &ns,
                        const std::string &name);
        bool is_element_mp(const xmlNode *ptr, const std::string &name);
        bool check_element_mp(const xmlNode *ptr, 
                               const std::string &name);
        std::string get_route(const xmlNode *node);

        const xmlNode* jump_to(const xmlNode* node, int node_type);

        const xmlNode* jump_to_next(const xmlNode* node, int node_type);
        
        const xmlNode* jump_to_children(const xmlNode* node, int node_type);

        void check_empty(const xmlNode *node);

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
