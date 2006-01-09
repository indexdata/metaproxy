/* $Id: filter_z3950_client.hpp,v 1.8 2006-01-09 18:19:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_Z3950_CLIENT_HPP
#define FILTER_Z3950_CLIENT_HPP

#include <boost/scoped_ptr.hpp>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Z3950Client : public Base {
            class Rep;
            class Assoc;
        public:
            ~Z3950Client();
            Z3950Client();
            void process(yp2::Package & package) const;
            void configure(const xmlNode * ptr);
        private:
            boost::scoped_ptr<Rep> m_p;
        };
    }
}

extern "C" {
    extern struct yp2_filter_struct yp2_filter_z3950_client;
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
