/* $Id: filter_z3950_client.hpp,v 1.1 2005-10-16 16:05:44 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_Z3950_CLIENT_HPP
#define FILTER_Z3950_CLIENT_HPP

#include <stdexcept>
#include <list>

#include "filter.hpp"

namespace yp2 {
    namespace filter {
        class Z3950Client : public Base {
            class Pimpl;
            class Assoc;
        public:
            ~Z3950Client();
            Z3950Client();
            void process(yp2::Package & package) const;
        private:
            Pimpl *m_p;
        };
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
