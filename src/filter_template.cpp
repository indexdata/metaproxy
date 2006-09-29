/* $Id: filter_template.cpp,v 1.9 2006-09-29 09:48:36 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"
#include "filter.hpp"
#include "filter_template.hpp"
#include "package.hpp"
#include "util.hpp"

#include <boost/thread/mutex.hpp>

#include <yaz/zgdu.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Template::Impl {
        public:
            Impl();
            ~Impl();
            void process(metaproxy_1::Package & package) const;
            void configure(const xmlNode * ptr);
        private:
            int m_dummy;
        };
    }
}

// define Pimpl wrapper forwarding to Impl
 
yf::Template::Template() : m_p(new Impl)
{
}

yf::Template::~Template()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Template::configure(const xmlNode *xmlnode)
{
    m_p->configure(xmlnode);
}

void yf::Template::process(mp::Package &package) const
{
    m_p->process(package);
}


// define Implementation stuff



yf::Template::Impl::Impl()
{
    m_dummy = 1;
}

yf::Template::Impl::~Impl()
{ 
}

void yf::Template::Impl::configure(const xmlNode *xmlnode)
{
}

void yf::Template::Impl::process(mp::Package &package) const
{
    // Z_GDU *gdu = package.request().get();
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
