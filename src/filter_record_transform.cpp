/* $Id: filter_record_transform.cpp,v 1.4 2006-10-05 12:17:24 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter.hpp"
#include "filter_record_transform.hpp"
#include "package.hpp"
#include "util.hpp"
#include "gduutil.hpp"
#include "xmlutil.hpp"

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

void yf::RecordTransform::configure(const xmlNode *xmlnode)
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
    Z_PresentRequest *pr = gdu_req->u.z3950->u.presentRequest;

    // setting up ODR's for memory during encoding/decoding
    //mp::odr odr_de(ODR_DECODE);  
    mp::odr odr_en(ODR_ENCODE);

    // setting up variables for conversion state
    yaz_record_conv_t rc = 0;
    int ret_code;

    const char *input_schema = 0;
    Odr_oid *input_syntax = 0;

    if(pr->recordComposition){
        input_schema 
            = mp_util::record_composition_to_esn(pr->recordComposition);
    }
    if(pr->preferredRecordSyntax){
        input_syntax = pr->preferredRecordSyntax;
    }
    
    const char *match_schema = 0;
    int *match_syntax = 0;

    const char *backend_schema = 0;
    Odr_oid *backend_syntax = 0;

    std::cout << "yaz_retrieval_request" << "\n";
    ret_code 
        = yaz_retrieval_request(m_retrieval,
                                input_schema, input_syntax,
                                &match_schema, &match_syntax,
                                &rc,
                                &backend_schema, &backend_syntax);

    std::cout << "ret_code " <<  ret_code << "\n";
    std::cout << "input   " << input_syntax << " ";
    if (input_syntax)
        std::cout << (oid_getentbyoid(input_syntax))->desc << " ";
    else
        std::cout << "- ";
    if (input_schema)
        std::cout   <<  input_schema << "\n";
    else
        std::cout   <<  "-\n";
    std::cout << "match   " << match_syntax << " ";
    if (match_syntax)
        std::cout << (oid_getentbyoid(match_syntax))->desc << " ";
    else
        std::cout << "- ";
    if (match_schema)
        std::cout   <<  match_schema << "\n";
    else
        std::cout   <<  "-\n";
    std::cout << "backend " << backend_syntax << " ";
    if (backend_syntax)
        std::cout << (oid_getentbyoid(backend_syntax))->desc << " ";
    else
        std::cout << "- ";
    if (backend_schema)
        std::cout   <<  backend_schema << "\n";
    else
        std::cout   <<  "-\n";
    
    // error handeling
    if (ret_code != 0)
    {

        // need to construct present error package and send back

        const char *details = 0;
        if (ret_code == -1) /* error ? */
        {
           details = yaz_retrieval_get_error(m_retrieval);
           std::cout << "ERROR: YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS"
                     << details << "\n";
           //rr->errcode = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
           // if (details)
           //     rr->errstring = odr_strdup(rr->stream, details);
        }
        else if (ret_code == 1 || ret_code == 3)
        {
            details = input_schema;
            std::cout << "ERROR: YAZ_BIB1_ELEMENT_SET_NAMES_UNSUPP"
                      << details << "\n";
            //rr->errcode =  YAZ_BIB1_ELEMENT_SET_NAMES_UNSUPP;
            //if (details)
            //    rr->errstring = odr_strdup(rr->stream, details);
        }
        else if (ret_code == 2)
        {
            std::cout << "ERROR: YAZ_BIB1_RECORD_SYNTAX_UNSUPP"
                      << details << "\n";
            //rr->errcode = YAZ_BIB1_RECORD_SYNTAX_UNSUPP;
            //if (input_syntax_raw)
            //{
            //    char oidbuf[OID_STR_MAX];
            //    oid_to_dotstring(input_syntax_raw, oidbuf);
            //    rr->errstring = odr_strdup(rr->stream, oidbuf);
            //}
        }
        //package.session().close();
        return;
    }



    // now re-coding the z3950 backend present request
     
    // z3950'fy record syntax

     if (backend_syntax)
         pr->preferredRecordSyntax
             = yaz_oidval_to_z3950oid(odr_en, CLASS_RECSYN, *backend_syntax);
     else
         pr->preferredRecordSyntax
             = yaz_oidval_to_z3950oid(odr_en, CLASS_RECSYN, VAL_NONE);

        
    //Odr_oid odr_oid;

        
        // = yaz_oidval_to_z3950oid (odr_en, CLASS_RECSYN, VAL_TEXT_XML);
    // }
    // Odr_oid *yaz_str_to_z3950oid (ODR o, int oid_class,
    //                                         const char *str);
    // const char *yaz_z3950oid_to_str (Odr_oid *oid, int *oid_class);

         //   oident *oident_syntax = oid_getentbyoid(backend_syntax);
         //
         //   rr->request_format_raw = backend_syntax;
         //   
         //   if (oident_syntax)
         //       rr->request_format = oident_syntax->value;
         //   else
         //       rr->request_format = VAL_NONE;
         



    // z3950'fy record schema
    if (backend_schema)
    {
        pr->recordComposition 
            = (Z_RecordComposition *) 
              odr_malloc(odr_en, sizeof(Z_RecordComposition));
        pr->recordComposition->which 
            = Z_RecordComp_simple;
        pr->recordComposition->u.simple 
            = (Z_ElementSetNames *)
               odr_malloc(odr_en, sizeof(Z_ElementSetNames));
        pr->recordComposition->u.simple->which = Z_ElementSetNames_generic;
        pr->recordComposition->u.simple->u.generic 
            = odr_strdup(odr_en, backend_schema);
    }

    // attaching Z3950 package to filter chain
    package.request() = gdu_req;

    // std::cout << "z3950_present_request " << *apdu << "\n";   

    // sending package
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

//         if (backend_schema)
//         {
//             set_esn(&rr->comp, backend_schema, rr->stream->mem);
//         }
//         if (backend_syntax)
//         {
//             oident *oident_syntax = oid_getentbyoid(backend_syntax);

//             rr->request_format_raw = backend_syntax;
            
//             if (oident_syntax)
//                 rr->request_format = oident_syntax->value;
//             else
//                 rr->request_format = VAL_NONE;

//        }
//     }
//     (*assoc->init->bend_fetch)(assoc->backend, rr);
//     if (rc && rr->record && rr->errcode == 0 && rr->len > 0)
//     {   /* post conversion must take place .. */
//         WRBUF output_record = wrbuf_alloc();
//         int r = yaz_record_conv_record(rc, rr->record, rr->len, output_record);
//         if (r)
//         {
//             const char *details = yaz_record_conv_get_error(rc);
//             rr->errcode = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
//             if (details)
//                 rr->errstring = odr_strdup(rr->stream, details);
//         }
//         else
//         {
//             rr->len = wrbuf_len(output_record);
//             rr->record = odr_malloc(rr->stream, rr->len);
//             memcpy(rr->record, wrbuf_buf(output_record), rr->len);
//         }
//         wrbuf_free(output_record, 1);
//     }
//     if (match_syntax)
//     {
//         struct oident *oi = oid_getentbyoid(match_syntax);
//         rr->output_format = oi ? oi->value : VAL_NONE;
//         rr->output_format_raw = match_syntax;
//     }
//     if (match_schema)
//         rr->schema = odr_strdup(rr->stream, match_schema);
//     return 0;

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
