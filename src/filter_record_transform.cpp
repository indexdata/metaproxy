/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

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
#include "filter.hpp"
#include "filter_record_transform.hpp"
#include "package.hpp"
#include "util.hpp"
#include "gduutil.hpp"
#include "xmlutil.hpp"

#include <yaz/diagbib1.h>
#include <yaz/zgdu.h>
#include <yaz/retrieval.h>

//#include <boost/thread/mutex.hpp>

#include <iostream>

namespace mp = metaproxy_1;
namespace yf = mp::filter;
namespace mp_util = metaproxy_1::util;

namespace metaproxy_1 {
    namespace filter {
        class RecordTransform::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * xml_node);
        private:
            yaz_retrieval_t m_retrieval;
        };
    }
}

// define Pimpl wrapper forwarding to Impl
 
yf::RecordTransform::RecordTransform() : m_p(new Impl)
{
}

yf::RecordTransform::~RecordTransform()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::RecordTransform::configure(const xmlNode *xmlnode, bool test_only)
{
    m_p->configure(xmlnode);
}

void yf::RecordTransform::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff



yf::RecordTransform::Impl::Impl() 
{
    m_retrieval = yaz_retrieval_create();
    assert(m_retrieval);
}

yf::RecordTransform::Impl::~Impl()
{ 
    if (m_retrieval)
        yaz_retrieval_destroy(m_retrieval);
}

void yf::RecordTransform::Impl::configure(const xmlNode *xml_node)
{
    //const char *srcdir = getenv("srcdir");
    //if (srcdir)
    //    yaz_retrieval_set_path(m_retrieval, srcdir);

    if (!xml_node)
        throw mp::XMLError("RecordTransform filter config: empty XML DOM");

    // parsing down to retrieval node, which can be any of the children nodes
    xmlNode *retrieval_node;
    for (retrieval_node = xml_node->children; 
         retrieval_node; 
         retrieval_node = retrieval_node->next)
    {
        if (retrieval_node->type != XML_ELEMENT_NODE)
            continue;
        if (0 == strcmp((const char *) retrieval_node->name, "retrievalinfo"))
            break;
    }

    // read configuration
    if ( 0 != yaz_retrieval_configure(m_retrieval, retrieval_node)){
        std::string msg("RecordTransform filter config: ");
        msg += yaz_retrieval_get_error(m_retrieval);
        throw mp::XMLError(msg);
    }
}

void yf::RecordTransform::Impl::process(mp::Package &package) const
{

    Z_GDU *gdu_req = package.request().get();
    
    // only working on z3950 present packages
    if (!gdu_req 
        || !(gdu_req->which == Z_GDU_Z3950) 
        || !(gdu_req->u.z3950->which == Z_APDU_presentRequest))
    {
        package.move();
        return;
    }
    
    // getting original present request
    Z_PresentRequest *pr_req = gdu_req->u.z3950->u.presentRequest;

    // setting up ODR's for memory during encoding/decoding
    //mp::odr odr_de(ODR_DECODE);  
    mp::odr odr_en(ODR_ENCODE);

    // setting up variables for conversion state
    yaz_record_conv_t rc = 0;
    int ret_code;

    const char *input_schema = 0;
    Odr_oid *input_syntax = 0;

    if(pr_req->recordComposition){
        input_schema 
            = mp_util::record_composition_to_esn(pr_req->recordComposition);
    }
    if(pr_req->preferredRecordSyntax){
        input_syntax = pr_req->preferredRecordSyntax;
    }
    
    const char *match_schema = 0;
    Odr_oid *match_syntax = 0;

    const char *backend_schema = 0;
    Odr_oid *backend_syntax = 0;

    ret_code 
        = yaz_retrieval_request(m_retrieval,
                                input_schema, input_syntax,
                                &match_schema, &match_syntax,
                                &rc,
                                &backend_schema, &backend_syntax);

    // error handling
    if (ret_code != 0)
    {
        // need to construct present error package and send back

        Z_APDU *apdu = 0;

        const char *details = 0;
        if (ret_code == -1) /* error ? */
        {
           details = yaz_retrieval_get_error(m_retrieval);
           apdu = odr_en.create_presentResponse(
               gdu_req->u.z3950,
               YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS, details);
        }
        else if (ret_code == 1 || ret_code == 3)
        {
            details = input_schema;
            apdu = odr_en.create_presentResponse(
                gdu_req->u.z3950,
                YAZ_BIB1_ELEMENT_SET_NAMES_UNSUPP, details);
        }
        else if (ret_code == 2)
        {
            char oidbuf[OID_STR_MAX];
            oid_oid_to_dotstring(input_syntax, oidbuf);
            details = odr_strdup(odr_en, oidbuf);
            
            apdu = odr_en.create_presentResponse(
                gdu_req->u.z3950,
                YAZ_BIB1_RECORD_SYNTAX_UNSUPP, details);
        }
        package.response() = apdu;
        return;
    }

    // now re-coding the z3950 backend present request
     
    if (backend_syntax) 
        pr_req->preferredRecordSyntax = odr_oiddup(odr_en, backend_syntax);
    else
        pr_req->preferredRecordSyntax = 0;
    

    // z3950'fy record schema
    if (backend_schema)
    {
        pr_req->recordComposition 
            = (Z_RecordComposition *) 
              odr_malloc(odr_en, sizeof(Z_RecordComposition));
        pr_req->recordComposition->which 
            = Z_RecordComp_simple;
        pr_req->recordComposition->u.simple 
            = (Z_ElementSetNames *)
               odr_malloc(odr_en, sizeof(Z_ElementSetNames));
        pr_req->recordComposition->u.simple->which = Z_ElementSetNames_generic;
        pr_req->recordComposition->u.simple->u.generic 
            = odr_strdup(odr_en, backend_schema);
    }

    // attaching Z3950 package to filter chain
    package.request() = gdu_req;

    package.move();

   //check successful Z3950 present response
    Z_GDU *gdu_res = package.response().get();
    if (!gdu_res || gdu_res->which != Z_GDU_Z3950 
        || gdu_res->u.z3950->which != Z_APDU_presentResponse
        || !gdu_res->u.z3950->u.presentResponse)

    {
        std::cout << "record-transform: error back present\n";
        package.session().close();
        return;
    }
    

    // everything fine, continuing
    // std::cout << "z3950_present_request OK\n";
    // std::cout << "back z3950 " << *gdu_res << "\n";

    Z_PresentResponse * pr_res = gdu_res->u.z3950->u.presentResponse;

    // let non surrogate dioagnostics in Z3950 present response package
    // pass to frontend - just return
    if (pr_res->records 
        && pr_res->records->which == Z_Records_NSD
        && pr_res->records->u.nonSurrogateDiagnostic)
    {
        // we might do more clever tricks to "reverse"
        // these error(s).

        //*pr_res->records->u.nonSurrogateDiagnostic->condition = 
        // YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }

    // record transformation must take place 
    if (rc && pr_res 
        && pr_res->numberOfRecordsReturned 
        && *(pr_res->numberOfRecordsReturned) > 0
        && pr_res->records
        && pr_res->records->which == Z_Records_DBOSD
        && pr_res->records->u.databaseOrSurDiagnostics->num_records)
    {
         //transform all records
         for (int i = 0; 
              i < pr_res->records->u.databaseOrSurDiagnostics->num_records; 
              i++)
         {
             Z_NamePlusRecord *npr 
                 = pr_res->records->u.databaseOrSurDiagnostics->records[i];
             if (npr->which == Z_NamePlusRecord_databaseRecord)
             {
                 WRBUF output_record = wrbuf_alloc();
                 Z_External *r = npr->u.databaseRecord;
                 int ret_trans = 0;
                 if (r->which == Z_External_OPAC)
                 {
#if YAZ_VERSIONL >= 0x030011
                     ret_trans = 
                         yaz_record_conv_opac_record(rc, r->u.opac,
                                                     output_record);
#else
                     ;
#endif
                 }
                 else if (r->which == Z_External_octet) 
                 {
                     ret_trans =
                         yaz_record_conv_record(rc, (const char *)
                                                r->u.octet_aligned->buf, 
                                                r->u.octet_aligned->len,
                                                output_record);
                 }
                 if (ret_trans == 0)
                 {
                     npr->u.databaseRecord =
                         z_ext_record_oid(odr_en, match_syntax,
                                          wrbuf_buf(output_record),
                                          wrbuf_len(output_record));
                 }
                 else
                 {
                     pr_res->records->
                         u.databaseOrSurDiagnostics->records[i] 
                         =  zget_surrogateDiagRec(
                             odr_en, npr->databaseName,
                             YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS,
                             yaz_record_conv_get_error(rc));
                 }
                 wrbuf_destroy(output_record);
             }
         }
    }
    package.response() = gdu_res;
    return;
}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::RecordTransform;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_record_transform = {
        0,
        "record_transform",
        filter_creator
    };
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
