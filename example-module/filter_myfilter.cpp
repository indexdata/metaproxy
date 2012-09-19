/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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

#include <yaz/log.h>
#include <yaz/diagbib1.h>

#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    namespace filter {
        class Filter_myfilter: public mp::filter::Base {
        public:
            void process(mp::Package & package) const;
            void configure(const xmlNode *ptr, bool test_only);
        };
    }
}

void mp::filter::Filter_myfilter::process(mp::Package & package) const
{   // See src/filter_backend_test.cpp for a more comprehensive
    // example of a dummy Z-server
    Z_GDU *gdu = package.request().get();
    Z_APDU *apdu_res = 0;
    mp::odr odr;

    if (!gdu || gdu->which != Z_GDU_Z3950)
    {
        yaz_log(YLOG_LOG, "myfilter::process: Not a Z39.50 packet");
        package.move(); // Send on to other filters
        return;
    }
    Z_APDU *apdu_req = gdu->u.z3950;
    if (apdu_req->which == Z_APDU_initRequest)
    {
        yaz_log(YLOG_LOG, "myfilter::process: Init request");
        apdu_res= odr.create_initResponse( apdu_req,
              YAZ_BIB1_PERMANENT_SYSTEM_ERROR, "Not implemented!");
        package.response() = apdu_res;
    }
    else
    {
        yaz_log(YLOG_LOG, "myfilter::process: Unknown request type");
        package.move(); // Send on to other filters
    }
}

void mp::filter::Filter_myfilter::configure(const xmlNode *ptr, bool test_only)
{
    yaz_log(YLOG_LOG, "myfilter::configure");
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "logmsg"))
        {
            std::string msg = mp::xml::get_text(ptr);
            yaz_log(YLOG_LOG, "myfilter::configure: %s", msg.c_str() );

        }
        else
        {
            throw mp::filter::FilterException("Bad element "
                      + std::string((const char *) ptr->name));
        }
    }


}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::Filter_myfilter;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_myfilter = {
        0,
        "myfilter",
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

