/* $Id: test_ses_map.cpp,v 1.3 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <iostream>
#include <stdexcept>

#include "router.hpp"
#include "session.hpp"
#include "package.hpp"

#include <map>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#define BOOST_AUTO_TEST_MAIN
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


BOOST_AUTO_UNIT_TEST( test_ses_map_1 )
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
