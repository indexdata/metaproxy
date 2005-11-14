/* $Id: filter_template.cpp,v 1.1 2005-11-14 23:35:22 adam Exp $
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

using namespace yp2;

yf::Template::Template() : m_p(new Rep)
{
}

yf::Template::~Template()
{
}

void yf::Template::process(Package &package) const
{
    // Z_GDU *gdu = package.request().get();
    package.move();
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
