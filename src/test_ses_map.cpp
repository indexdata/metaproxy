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

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include <metaproxy/package.hpp>

#include <map>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

#include <yaz/zgdu.h>
#include <yaz/pquery.h>
#include <yaz/otherinfo.h>
using namespace boost::unit_test;

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    class SesMap;


    class SesMap {
        class Wrap {
        public:
            Wrap(const double &t) : m_t(t) { };
            double m_t;
            boost::mutex m_mutex;
        };
    private:
        boost::mutex m_map_mutex;
    public:
        void create(SesMap &sm, const mp::Session &s, double &t) {
            boost::mutex::scoped_lock lock(m_map_mutex);

            boost::shared_ptr<Wrap> w_ptr(new Wrap(t));
            m_map_ptr[s] = w_ptr;
        }
        std::map<mp::Session,boost::shared_ptr<Wrap> >m_map_ptr;
    };
}


BOOST_AUTO_TEST_CASE( test_ses_map_1 )
{
    try
    {
        mp::SesMap ses_map;
    }
    catch ( ... ) {
        BOOST_CHECK (false);
    }
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

