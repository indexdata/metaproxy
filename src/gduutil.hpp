/* This file is part of Metaproxy.
   Copyright (C) 2005-2009 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

