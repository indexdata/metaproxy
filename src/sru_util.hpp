/* $Id: sru_util.hpp,v 1.1 2006-09-26 13:15:33 marc Exp $
   Copyright (c) 2005-2006, Index Data.

   See the LICENSE file for details
 */

#ifndef YP2_SDU_UTIL_HPP
#define YP2_SDU_UTIL_HPP

//#include <yaz/zgdu.h>
//#include <yaz/z-core.h>
#include <yaz/srw.h>

#include <iosfwd>

namespace std 
{
    std::ostream& operator<<(std::ostream& os, Z_SRW_PDU& srw_pdu); 

}


namespace metaproxy_1 {
    namespace sru  {

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
