/* $Id: test_boost_threads.cpp,v 1.7 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
 */

#include "config.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include <list>
#include <iostream>

class counter
{
   public:
      counter() : count(0) { }
      
    int increment() {
        boost::mutex::scoped_lock scoped_lock(mutex);
        return ++count;
    }
    
private:
    boost::mutex mutex;
    int count;
};


counter c;

class worker {
public:
    void operator() (void) {
        c.increment();
    }
};

#define USE_GROUP 1


BOOST_AUTO_UNIT_TEST( thread_group )
{
    try 
    {
        const int num_threads = 4;
        boost::thread_group thrds;
        
        for (int i=0; i < num_threads; ++i)
        {
            worker w;
            thrds.add_thread(new boost::thread(w));
        }
        thrds.join_all();
    }
    catch (...) 
    {
        BOOST_CHECK(false);
    }
    BOOST_CHECK(c.increment() == 5);
}


BOOST_AUTO_UNIT_TEST( thread_list )
{
    try 
    {
        const int num_threads = 4;
        std::list<boost::thread *> thread_list;
        
        for (int i=0; i < num_threads; ++i)
        {
            worker w;
            thread_list.push_back(new boost::thread(w));
        }
        std::list<boost::thread *>::iterator it;
        for (it = thread_list.begin(); it != thread_list.end(); it++)
        {
            (*it)->join();
            delete *it;
        }

    }
    catch (...) 
    {
        BOOST_CHECK(false);
    }
    BOOST_CHECK(c.increment() == 10);
}



/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
