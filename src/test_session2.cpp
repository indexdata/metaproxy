/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

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
#include <metaproxy/session.hpp>

#include <iostream>
#include <list>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

boost::mutex io_mutex;

class Worker 
{
    public:
        Worker(int nr = 0) 
            :  m_nr(nr), m_id(0) {};
        
        void operator() (void) {
            for (int i=0; i < 100; ++i)
            {
                mp::Session session;
                m_id = session.id();   
                //print();
            }
        }

        void print()
        {
            boost::mutex::scoped_lock scoped_lock(io_mutex);
            std::cout << "Worker " << m_nr 
                      << " session.id() " << m_id << std::endl;
        }
        
    private: 
        int m_nr;
        int m_id;
};



BOOST_AUTO_TEST_CASE( testsession2 ) 
{

    // test session 
    try {

        const int num_threads = 100;
        boost::thread_group thrds;
        

        for (int i=0; i < num_threads; ++i)
        {
            // Notice that each Worker has it's own session object!
            Worker w(i);
            thrds.add_thread(new boost::thread(w));
        }
        thrds.join_all();

        mp::Session session;
        BOOST_CHECK (session.id() == 10001);
        
    }
    catch (std::exception &e) {
        std::cout << e.what() << "\n";
        BOOST_CHECK (false);
    }
    catch (...) {
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

