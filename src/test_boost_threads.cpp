
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <list>
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


class worker {
public:
    void operator() (void) {
        int i = c.increment();
        
        i = c.increment();
        
        i = c.increment();
        boost::mutex::scoped_lock scoped_lock(io_mutex);
        std::cout << "count == " << i << std::endl;
    }
    virtual ~worker() { std::cout << "destroyed\n"; }
};



int main(int, char*[])
{
   try 
   {
      const int num_threads = 4;
      boost::thread_group thrds;
      
      std::list<boost::thread *> thread_list;
      
      for (int i=0; i < num_threads; ++i)
      {
          // thrds.create_thread(&change_count);
          worker *w = new worker;

          boost::thread *thr = new boost::thread(*w);

          thrds.add_thread(thr);

          thread_list.push_back(thr);
      }
      
      thrds.join_all();
#if 0
      std::list<boost::thread *>::iterator it;
      for (it = thread_list.begin(); it != thread_list.end(); it++)
      {
          delete *it;
          *it = 0;
      }
#endif
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
