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

        std::string url_recipe_handle(xmlDoc *doc, std::string recipe);
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

