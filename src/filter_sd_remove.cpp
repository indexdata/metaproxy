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

#include "config.hpp"
#include "filter_sd_remove.hpp"
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>
#include <yaz/zgdu.h>
#include <metaproxy/filter.hpp>
#include <yaz/querytowrbuf.h>

namespace metaproxy_1 {
    namespace filter {
        class SD_Remove : public Base {
        public:
            SD_Remove();
            ~SD_Remove();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr, bool test_only,
                           const char *path);
        };
    }
}

namespace mp = metaproxy_1;
namespace yf = mp::filter;

yf::SD_Remove::SD_Remove()
{
}

yf::SD_Remove::~SD_Remove()
{
}

void yf::SD_Remove::configure(const xmlNode *xmlnode, bool test_only,
                             const char *path)
{
    if (xmlnode)
    {
        xmlNode *ptr;
        for (ptr = xmlnode->children; ptr; ptr = ptr->next)
        {
            if (ptr->type == XML_ELEMENT_NODE)
                throw mp::filter::FilterException("Bad element "
                                                  + std::string((const char *)
                                                                ptr->name));
        }
    }
}

void yf::SD_Remove::process(mp::Package &package) const
{
    package.move();

    Z_GDU *gdu_res = package.response().get();
    if (gdu_res && gdu_res->which == Z_GDU_Z3950)
    {
        Z_NamePlusRecordList *records = 0;
        Z_APDU *apdu = gdu_res->u.z3950;
        if (apdu->which == Z_APDU_presentResponse)
        {
            Z_PresentResponse * pr_res = apdu->u.presentResponse;
            if (pr_res->numberOfRecordsReturned
                && *(pr_res->numberOfRecordsReturned) > 0
                && pr_res->records
                && pr_res->records->which == Z_Records_DBOSD)
            {
                records = pr_res->records->u.databaseOrSurDiagnostics;
            }
        }
        if (apdu->which == Z_APDU_searchResponse)
        {
            Z_SearchResponse *sr_res = apdu->u.searchResponse;
            if (
                sr_res->numberOfRecordsReturned
                && *(sr_res->numberOfRecordsReturned) > 0
                && sr_res->records
                && sr_res->records->which == Z_Records_DBOSD)
            {
                records = sr_res->records->u.databaseOrSurDiagnostics;
            }
        }
        if (records)
        {
            mp::odr odr_en(ODR_ENCODE);
            int i;
            for (i = 0; i < records->num_records; i++)
            {
                Z_NamePlusRecord *npr = records->records[i];
                if (npr->which == Z_NamePlusRecord_surrogateDiagnostic)
                {
                    WRBUF w = wrbuf_alloc();
                    wrbuf_diags(w, 1, &npr->u.surrogateDiagnostic);
                    npr->which = Z_NamePlusRecord_databaseRecord;
                    npr->u.databaseRecord = z_ext_record_sutrs(odr_en,
                                                               wrbuf_buf(w),
                                                               wrbuf_len(w));
                    wrbuf_destroy(w);
                }
            }
            package.response() = gdu_res;
        }
    }
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::SD_Remove;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_sd_remove = {
        0,
        "sd_remove",
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

