/* $Id: test_session2.cpp,v 1.9 2007-01-25 14:05:54 adam Exp $
   Copyright (c) 2005-2007, Index Data.

   See the LICENSE file for details
 */
#include "config.hpp"
#include "session.hpp"

#include <iostream>
#include <list>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;
namespace mp = metaproxy_1;

boost::mutex io_mutex;

class Worker 
{
    public:
        Worker(int nr = 0) 
            :  m_nr(nr){};
        
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



BOOST_AUTO_UNIT_TEST( testsession2 ) 
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
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
