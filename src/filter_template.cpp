/* $Id: filter_template.cpp,v 1.8 2006-06-10 14:29:12 adam Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#include "config.hpp"

#include "filter.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "util.hpp"
#include "filter_template.hpp"

#include <yaz/zgdu.h>

namespace mp = metaproxy_1;
namespace yf = mp::filter;

namespace metaproxy_1 {
    namespace filter {
        class Template::Rep {
            friend class Template;
            int dummy;
        };
    }
}

yf::Template::Template() : m_p(new Rep)
{
    m_p->dummy = 1;
}

yf::Template::~Template()
{  // must have a destructor because of boost::scoped_ptr
}

void yf::Template::process(mp::Package &package) const
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
