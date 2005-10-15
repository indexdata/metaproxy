
#ifndef FILTER_LOG_HPP
#define FILTER_LOG_HPP

#include <stdexcept>
#include <vector>

#include "filter.hpp"

namespace yp2 {
    class FilterLog : public yp2::Filter {
    public:
	FilterLog::FilterLog();
	void process(yp2::Package & package) const;
    };
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
