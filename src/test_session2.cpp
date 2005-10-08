#include "session.hpp"

#include <iostream>
#include <list>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;

boost::mutex io_mutex;

class Worker 
{
    public:
        Worker(yp2::Session *session, int nr = 0) 
            : m_session(session), m_nr(nr){};
        
        void operator() (void) {
            for (int i=0; i < 100; ++i)
            {
                m_id = m_session->id();   
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
        yp2::Session *m_session;
        int m_nr;
        int m_id;
};



BOOST_AUTO_TEST_CASE( testsession2 ) 
{

    // test session 
    try {
        yp2::Session session;

        const int num_threads = 10;
        boost::thread_group thrds;
        
        for (int i=0; i < num_threads; ++i)
        {
            Worker w(&session, i);
            thrds.add_thread(new boost::thread(w));
        }
        thrds.join_all();

        BOOST_CHECK (session.id() == 1001);
        
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
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
