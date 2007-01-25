/* $Id: gduutil.hpp,v 1.5 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_GDUUTIL_HPP
#define YP2_GDUUTIL_HPP

#include <yaz/zgdu.h>
#include <yaz/z-core.h>
//#include <yaz/srw.h>

#include <iosfwd>

namespace std 
{
    std::ostream& operator<<(std::ostream& os, Z_GDU& zgdu);
    std::ostream& operator<<(std::ostream& os, Z_APDU& zapdu); 
    std::ostream& operator<<(std::ostream& os, Z_HTTP_Request& httpreq);
    std::ostream& operator<<(std::ostream& os, Z_HTTP_Response& httpres);
    std::ostream& operator<<(std::ostream& os, Z_Records & rs);
    std::ostream& operator<<(std::ostream& os, Z_DiagRec& dr);
    std::ostream& operator<<(std::ostream& os, Z_DefaultDiagFormat& ddf);
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
