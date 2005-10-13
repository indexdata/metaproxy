
#include <stdexcept>

#include "session.hpp"
#include <boost/thread/mutex.hpp>

#include "config.hpp"

// defining and initializing static members
boost::mutex yp2::Session::m_mutex;
unsigned long int yp2::Session::m_global_id = 0;

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
