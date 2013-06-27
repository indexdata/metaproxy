/* This file is part of Metaproxy.
   Copyright (C) 2005-2013 Index Data

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

#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <boost/scoped_ptr.hpp>

namespace metaproxy_1 {
        class HTMLParserEvent {
        public:
            virtual void openTagStart(const char *tag, int tag_len) = 0;
            virtual void anyTagEnd(const char *tag, int tag_len,
                                   int close_it) = 0;
            virtual void attribute(const char *tag, int tag_len,
                                   const char *attr, int attr_len,
                                   const char *value, int val_len) = 0;
            virtual void closeTag(const char *tag, int tag_len) = 0;
            virtual void text(const char *value, int len) = 0;
        };
        class HTMLParser {
            class Rep;
        public:
            HTMLParser();
            ~HTMLParser();
            void parse(HTMLParserEvent &event, const char *str) const;
            void set_verbose(int v);
        private:
            boost::scoped_ptr<Rep> m_p;
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

