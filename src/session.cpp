/* $Id: session.cpp,v 1.2 2005-10-15 14:09:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

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
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
