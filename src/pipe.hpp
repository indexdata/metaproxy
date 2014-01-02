/* This file is part of Metaproxy.
   Copyright (C) Index Data

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

#ifndef YP2_PIPE_HPP
#define YP2_PIPE_HPP

#include <stdexcept>
#include <string>
#include <boost/scoped_ptr.hpp>

#include <yaz/yconfig.h>

namespace metaproxy_1 {
    class Pipe {
        class Error : public std::runtime_error {
        public:
            Error(const std::string msg)
                : std::runtime_error("Pipe error: " + msg) {};
        };
        class Rep;
    public:
        Pipe(int port_to_use);
        ~Pipe();
        int &read_fd() const;
        int &write_fd() const;
    private:
        boost::scoped_ptr<Rep> m_p;
    };
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

