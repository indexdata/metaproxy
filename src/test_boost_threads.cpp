
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

boost::mutex io_mutex; // The iostreams are not guaranteed to be thread-safe!

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

void change_count()
{
   int i = c.increment();
   boost::mutex::scoped_lock scoped_lock(io_mutex);
   std::cout << "count == " << i << std::endl;
}



int main(int, char*[])
{
   try 
   {
      const int num_threads = 4;
      boost::thread_group thrds;
      for (int i=0; i < num_threads; ++i)
         thrds.create_thread(&change_count);
      
      thrds.join_all();
      
   }
   catch (std::exception &e) 
   {
      std::cout << e.what() << "\n";
      exit(1);
   }
   exit(0);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
