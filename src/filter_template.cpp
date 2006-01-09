/* $Id: filter_template.cpp,v 1.5 2006-01-09 14:47:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter.hpp"
#include "router.hpp"
#include "package.hpp"

#include <boost/thread/mutex.hpp>

#include "util.hpp"
#include "filter_template.hpp"

#include <yaz/zgdu.h>

namespace yf = yp2::filter;

namespace yp2 {
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

void yf::Template::process(yp2::Package &package) const
{
    // Z_GDU *gdu = package.request().get();
    package.move();
}

static yp2::filter::Base* filter_creator()
{
    return new yp2::filter::Template;
}

extern "C" {
    struct yp2_filter_struct yp2_filter_template = {
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
