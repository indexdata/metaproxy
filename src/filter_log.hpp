/* $Id: filter_log.hpp,v 1.8 2005-10-29 22:23:36 marc Exp $
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
            Log(const std::string &msg);
            Log();
            void process(yp2::Package & package) const;
            const std::string type() const {
                return "Log";
            };
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
