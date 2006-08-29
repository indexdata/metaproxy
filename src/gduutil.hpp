/* $Id: gduutil.hpp,v 1.1 2006-08-29 10:06:31 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_GDUUTIL_HPP
#define YP2_GDUUTIL_HPP

#include <yaz/zgdu.h>
#include <yaz/z-core.h>

#include <iosfwd>

namespace std 
{
    std::ostream& operator<<(std::ostream& os, Z_GDU& zgdu);
    std::ostream& operator<<(std::ostream& os, Z_APDU& zapdu);    
}


namespace metaproxy_1 {
    namespace gdu  {


    }    
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
