#include <iostream>

#include "boost/date_time/posix_time/posix_time.hpp"

#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

using namespace boost::unit_test;



BOOST_AUTO_TEST_CASE( testboosttime1 ) 
{

    // test session 
    try {

        //using namespace boost::posix_time;
  //using namespace boost::gregorian;

  //get the current time from the clock -- one second resolution
  //boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
  //std::cout << to_iso_extended_string(now) << std::endl;
  //std::cout << now << std::endl;

  sleep(1);
  
  boost::posix_time::ptime then = boost::posix_time::microsec_clock::local_time();

  //std::cout << then << std::endl;

  boost::posix_time::time_period period(now, then);
  
  //std::cout << period << std::endl;


  //Get the date part out of the time
  //date today = now.date();
  //date tommorrow = today + days(1);
  //ptime tommorrow_start(tommorrow); //midnight 

  //iterator adds by one hour
  //time_iterator titr(now,hours(1)); 
  //for (; titr < tommorrow_start; ++titr) {
  //  std::cout << to_simple_string(*titr) << std::endl;
  //}
  
  //time_duration remaining = tommorrow_start - now;
  //std::cout << "Time left till midnight: " 
  //          << to_simple_string(remaining) << std::endl;

        BOOST_CHECK (1 == 1);
        
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
