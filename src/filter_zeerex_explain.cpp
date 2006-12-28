/* $Id: filter_zeerex_explain.cpp,v 1.1 2006-12-28 14:59:44 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"
#include "util.hpp"
#include "gduutil.hpp"
#include "sru_util.hpp"
#include "filter_zeerex_explain.hpp"

#include <yaz/zgdu.h>
#include <yaz/z-core.h>
#include <yaz/srw.h>
#include <yaz/pquery.h>

#include <boost/thread/mutex.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

namespace mp = metaproxy_1;
namespace mp_util = metaproxy_1::util;
namespace yf = mp::filter;


namespace metaproxy_1 {
    namespace filter {
        class ZeeRexExplain::Impl {
        public:
            void configure(const xmlNode *xmlnode);
            void process(metaproxy_1::Package &package) const;
        private:
        };
    }
}

yf::ZeeRexExplain::ZeeRexExplain() : m_p(new Impl)
{
}

yf::ZeeRexExplain::~ZeeRexExplain()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::ZeeRexExplain::configure(const xmlNode *xmlnode)
{
    m_p->configure(xmlnode);
}

void yf::ZeeRexExplain::process(mp::Package &package) const
{
    m_p->process(package);
}

void yf::ZeeRexExplain::Impl::configure(const xmlNode *xmlnode)
{
}

void yf::ZeeRexExplain::Impl::process(mp::Package &package) const
{
    Z_GDU *zgdu_req = package.request().get();

    // ignoring all non HTTP_Request packages
    if (!zgdu_req || !(zgdu_req->which == Z_GDU_HTTP_Request)){
        package.move();
        return;
    }
    
    // only working on  HTTP_Request packages now

    mp::odr odr_de(ODR_DECODE);
    Z_SRW_PDU *sru_pdu_req = 0;

    mp::odr odr_en(ODR_ENCODE);
    //Z_SRW_PDU *sru_pdu_res = 0;
    Z_SRW_PDU *sru_pdu_res = yaz_srw_get(odr_en, Z_SRW_explain_response);

    Z_SOAP *soap = 0;
    char *charset = 0;
    char *stylesheet = 0;


    // if SRU package could not be decoded, send minimal explain and
    // close connection
    if (! (sru_pdu_req = mp_util::decode_sru_request(package, odr_de, odr_en, 
                                            sru_pdu_res, soap,
                                            charset, stylesheet)))
    {
        mp_util::build_simple_explain(package, odr_en, sru_pdu_res, 0);
        mp_util::build_sru_response(package, odr_en, soap, 
                           sru_pdu_res, charset, stylesheet);
        package.session().close();
        return;
    }
    
    
    // SRU request package translation to Z3950 package
    //if (sru_pdu_req)
    //    std::cout << *sru_pdu_req << "\n";
    //else
    //    std::cout << "SRU empty\n";
    

    if (sru_pdu_req->which != Z_SRW_explain_request){
    // Let pass all other SRU actions
        package.move();
        return;
    }
    // except valid SRU explain request, construct ZeeRex Explain response
    else {
        Z_SRW_explainRequest *er_req = sru_pdu_req->u.explain_request;

        mp_util::build_simple_explain(package, odr_en, sru_pdu_res, er_req);

        mp_util::build_sru_response(package, odr_en, soap, 
                                    sru_pdu_res, charset, stylesheet);

        return;
    }

    // should never arrive here
    package.session().close();
    return;   
}



static mp::filter::Base* filter_creator()
{
    return new mp::filter::ZeeRexExplain;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_zeerex_explain = {
        0,
        "zeerex_explain",
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
