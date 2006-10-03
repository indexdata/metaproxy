/* $Id: filter_record_transform.cpp,v 1.1 2006-10-03 14:04:22 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter.hpp"
#include "filter_record_transform.hpp"
#include "package.hpp"
#include "util.hpp"
#include "gduutil.hpp"

#include <yaz/zgdu.h>

//#include <boost/thread/mutex.hpp>

#include <iostream>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class RecordTransform::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * xml_node);
        private:

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
}

yf::RecordTransform::Impl::~Impl()
{ 
}

void yf::RecordTransform::Impl::configure(const xmlNode *xml_node)
{
//    for (xml_node = xml_node->children; xml_node; xml_node = xml_node->next)
//     {
//         if (xml_node->type != XML_ELEMENT_NODE)
//             continue;
//         if (!strcmp((const char *) xml_node->name, "target"))
//         {
//             std::string route = mp::xml::get_route(xml_node);
//             std::string target = mp::xml::get_text(xml_node);
//             std::cout << "route=" << route << " target=" << target << "\n";
//             m_p->m_target_route[target] = route;
//         }
//         else if (!strcmp((const char *) xml_node->name, "hideunavailable"))
//         {
//             m_p->m_hide_unavailable = true;
//         }
//         else
//         {
//             throw mp::filter::FilterException
//                 ("Bad element " 
//                  + std::string((const char *) xml_node->name)
//                  + " in virt_db filter");
//         }
//     }
}

void yf::RecordTransform::Impl::process(mp::Package &package) const
{

    Z_GDU *gdu = package.request().get();
    
    // only working on z3950 present packages
    if (!gdu 
        || !(gdu->which == Z_GDU_Z3950) 
        || !(gdu->u.z3950->which == Z_APDU_presentRequest))
    {
        package.move();
        return;
    }
    
    // getting original present request
    Z_PresentRequest *front_pr = gdu->u.z3950->u.presentRequest;

    // setting up ODR's for memory during encoding/decoding
    mp::odr odr_de(ODR_DECODE);  
    mp::odr odr_en(ODR_ENCODE);

    // now packaging the z3950 backend present request
    Package back_package(package.session(), package.origin());
    back_package.copy_filter(package);
 
    Z_APDU *apdu = zget_APDU(odr_en, Z_APDU_presentRequest);

    assert(apdu->u.presentRequest);

    // z3950'fy start record position
    //if ()
    //    *(apdu->u.presentRequest->resultSetStartPoint) 
    //        =
    
    // z3950'fy number of records requested 
    //if ()
    //    *(apdu->u.presentRequest->numberOfRecordsRequested) 
     
    // z3950'fy record syntax
    //if()
    //(apdu->u.presentRequest->preferredRecordSyntax)
    //    = yaz_oidval_to_z3950oid (odr_en, CLASS_RECSYN, VAL_TEXT_XML);

    // z3950'fy record schema
    //if ()
    // {
    //     apdu->u.presentRequest->recordComposition 
    //         = (Z_RecordComposition *) 
    //           odr_malloc(odr_en, sizeof(Z_RecordComposition));
    //     apdu->u.presentRequest->recordComposition->which 
    //         = Z_RecordComp_simple;
    //     apdu->u.presentRequest->recordComposition->u.simple 
    //         = build_esn_from_schema(odr_en, 
    //                                 (const char *) sr_req->recordSchema); 
    // }

    // attaching Z3950 package to filter chain
    back_package.request() = apdu;

    // std::cout << "z3950_present_request " << *apdu << "\n";   

    // sending backend package
    back_package.move();

   //check successful Z3950 present response
    Z_GDU *back_gdu = back_package.response().get();
    if (!back_gdu || back_gdu->which != Z_GDU_Z3950 
        || back_gdu->u.z3950->which != Z_APDU_presentResponse
        || !back_gdu->u.z3950->u.presentResponse)

    {
        std::cout << "record-transform: error back present\n";
        package.session().close();
        return;
    }
    

    // everything fine, continuing
    // std::cout << "z3950_present_request OK\n";
    // std::cout << "back z3950 " << *back_gdu << "\n";


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
