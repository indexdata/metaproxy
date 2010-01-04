/* This file is part of Metaproxy.
   Copyright (C) 2005-2010 Index Data

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
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
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


BOOST_AUTO_TEST_CASE( thread_group )
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


BOOST_AUTO_TEST_CASE( thread_list )
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

