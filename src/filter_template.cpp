/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

#include "config.hpp"
#include "filter_template.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <yaz/zgdu.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Template : public Base {
        public:
            Template();
            ~Template();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        private:
            int m_dummy;
        };
    }
}

yf::Template::Template()
{
}

yf::Template::~Template()
{
}

void yf::Template::configure(const xmlNode *xmlnode, bool test_only,
                             const char *path)
{
}

void yf::Template::process(mp::Package &package) const
{
    package.move();
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::Template;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_template = {
        0,
        "template",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

