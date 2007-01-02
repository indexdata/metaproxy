/* $Id: filter_load_balance.cpp,v 1.1 2007-01-02 15:35:36 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter.hpp"
#include "filter_load_balance.hpp"
#include "package.hpp"
#include "util.hpp"

#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <yaz/zgdu.h>

#include <iostream>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class LoadBalance::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package);
            void configure(const xmlNode * ptr);
        private:
            boost::mutex m_mutex;
        };
    }
}

// define Pimpl wrapper forwarding to Impl
 
yf::LoadBalance::LoadBalance() : m_p(new Impl)
{
}

yf::LoadBalance::~LoadBalance()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::LoadBalance::configure(const xmlNode *xmlnode)
{
    m_p->configure(xmlnode);
}

void yf::LoadBalance::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff



yf::LoadBalance::Impl::Impl()
{
}

yf::LoadBalance::Impl::~Impl()
{ 
}

void yf::LoadBalance::Impl::configure(const xmlNode *xmlnode)
{
}

void yf::LoadBalance::Impl::process(mp::Package &package)
{
    Z_GDU *gdu_req = package.request().get();


    // passing anything but z3950 packages
    if (!gdu_req 
        || !(gdu_req->which == Z_GDU_Z3950))
    {
        package.move();
        return;
    }


    // target selecting only on Z39.50 init request
    if (gdu_req->u.z3950->which == Z_APDU_initRequest){

        mp::odr odr_en(ODR_ENCODE);
        Z_InitRequest *org_init = gdu_req->u.z3950->u.initRequest;

        // extracting virtual hosts
        std::list<std::string> vhosts;

        mp::util::remove_vhost_otherinfo(&(org_init->otherInfo), vhosts);

        //std::cout << "LoadBalance::Impl::process() vhosts: " 
        //          << vhosts.size()  << "\n";
        //std::cout << "LoadBalance::Impl::process()" << *gdu_req << "\n";
        
        // choosing one target according to load-balancing algorithm  
        
        if (vhosts.size()){
            std::string target;
            
            // getting timestamp for receiving of package
            boost::posix_time::ptime receive_time
                = boost::posix_time::microsec_clock::local_time();
            
            // //<< receive_time << " "
            // //<< to_iso_string(receive_time) << " "
            //<< to_iso_extended_string(receive_time) << " "
            // package.session().id();
            
            { // scope for locking local target database  
                boost::mutex::scoped_lock scoped_lock(m_mutex);
                target = *vhosts.begin();
            }
            
            
            // copying new target into init package
            mp::util::set_vhost_otherinfo(&(org_init->otherInfo), odr_en, target); 
            package.request() = gdu_req;
        }
        
    }
    
        
    // moving all Z39.50 package typess 
    package.move();
        


    //boost::posix_time::ptime send_time
    //    = boost::posix_time::microsec_clock::local_time();

    //boost::posix_time::time_duration duration = send_time - receive_time;


    //    { // scope for locking local target database  
    //        boost::mutex::scoped_lock scoped_lock(m_mutex);
    //        target = *vhosts.begin();
    //    }


}


static mp::filter::Base* filter_creator()
{
    return new mp::filter::LoadBalance;
}

extern "C" {
    struct metaproxy_1_filter_struct metaproxy_1_filter_load_balance = {
        0,
        "load_balance",
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
