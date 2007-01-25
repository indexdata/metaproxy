/* $Id: pipe.hpp,v 1.6 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

