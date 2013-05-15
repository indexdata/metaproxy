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
#include "filter_present_chunk.hpp"

#include <time.h>
#include <yaz/log.h>
#include <yaz/copy_types.h>
#include <metaproxy/package.hpp>
#include <metaproxy/util.hpp>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class PresentChunk::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
            void chunk_it(metaproxy_1::Package & package, Z_APDU *apdu);
        private:
            Odr_int chunk_number;
        };
    }
}

yf::PresentChunk::PresentChunk() : m_p(new Impl)
{
}

yf::PresentChunk::~PresentChunk()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::PresentChunk::configure(const xmlNode *xmlnode, bool test_only,
                          const char *path)
{
    m_p->configure(xmlnode);
}

void yf::PresentChunk::process(mp::Package &package) const
{
    m_p->process(package);
}

yf::PresentChunk::Impl::Impl() : chunk_number(0)
{
}

yf::PresentChunk::Impl::~Impl()
{
}

void yf::PresentChunk::Impl::configure(const xmlNode *ptr)
{
    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "chunk"))
        {
            chunk_number = mp::xml::get_int(ptr, 0);
        }
        else
        {
            throw mp::filter::FilterException("Bad element "
                                               + std::string((const char *)
                                                             ptr->name));
        }
    }
}

void yf::PresentChunk::Impl::chunk_it(mp::Package &package,
                                      Z_APDU *apdu)
{
    mp::odr odr;
    Z_PresentRequest *pr = apdu->u.presentRequest;

    Odr_int total = *pr->numberOfRecordsRequested;
    Odr_int start = *pr->resultSetStartPoint;
    Odr_int offset = 0;
    Z_NamePlusRecordList *npl = (Z_NamePlusRecordList *)
        odr_malloc(odr, sizeof(*npl));
    npl->num_records = total;
    npl->records = (Z_NamePlusRecord **)
        odr_malloc(odr, sizeof(*npl->records) * total);
    while (offset < total)
    {
        Odr_int left = total - offset;

        Package pp(package.session(), package.origin());

        *pr->numberOfRecordsRequested =
            left > chunk_number ? chunk_number : left;

        *pr->resultSetStartPoint = start + offset;

        pp.copy_filter(package);
        pp.request() = apdu;

        pp.move();

        if (pp.session().is_closed())
        {
            package.session().close();
            return;
        }
        Z_GDU *gdu_res = pp.response().get();
        if (gdu_res && gdu_res->which == Z_GDU_Z3950 &&
            gdu_res->u.z3950->which == Z_APDU_presentResponse)
        {
            Z_PresentResponse *pres =
                gdu_res->u.z3950->u.presentResponse;
            if (pres->records &&
                pres->records->which == Z_Records_DBOSD)
            {
                Z_NamePlusRecordList *nprl =
                    pres->records->u.databaseOrSurDiagnostics;
                int i;
                for (i = 0; i < nprl->num_records; i++)
                {
                    ODR o = odr;
                    npl->records[offset+i] = yaz_clone_z_NamePlusRecord(
                        nprl->records[i], o->mem);
                }
                offset += nprl->num_records;
            }
            else
            {
                package.response() = pp.response();
                return;
            }
        }
        else
        {
            package.response() = pp.response();
            return;
        }
    }

    yaz_log(YLOG_LOG, "building response . %lld", offset);

    Z_APDU *a = zget_APDU(odr, Z_APDU_presentResponse);
    Z_PresentResponse *pres = a->u.presentResponse;
    pres->records = (Z_Records *)
        odr_malloc(odr, sizeof(Z_Records));
    pres->records->which = Z_Records_DBOSD;
    pres->records->u.databaseOrSurDiagnostics = npl;
    npl->num_records = offset;
    *pres->numberOfRecordsReturned = offset;

    package.response() = a;
}

void yf::PresentChunk::Impl::process(mp::Package &package)
{
    Z_GDU *gdu = package.request().get();
    if (gdu && gdu->which == Z_GDU_Z3950)
    {
        Z_APDU *apdu = gdu->u.z3950;
        if (apdu->which == Z_APDU_presentRequest && chunk_number > 0)
            chunk_it(package, apdu);
        else
            package.move();
    }
    else
        package.move();
}

static mp::filter::Base* filter_creator()
{
    return new mp::filter::PresentChunk;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_present_chunk = {
        0,
        "present_chunk",
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

