/* $Id: filter_log.hpp,v 1.6 2005-10-25 11:48:30 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_LOG_HPP
#define FILTER_LOG_HPP

#include <stdexcept>
#include <vector>

#include "filter.hpp"

#include <boost/thread/mutex.hpp>


namespace yp2 {
    namespace filter {
        class Log : public Base {
        public:
            Log();
            void process(yp2::Package & package) const;
            void set_prefix(const std::string &msg);
        private:
            /// static mutex to lock Ostream during logging operation
            static boost::mutex m_log_mutex;
            std::string m_msg;
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
