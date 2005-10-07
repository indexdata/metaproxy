
#ifndef SESSION_HPP
#define SESSION_HPP

#include <stdexcept>


#include <boost/thread/mutex.hpp>

namespace yp2 {

  class Session 
    {
    public:
      Session() : m_id(0){};
      /// returns next id, global state of id protected by boost::mutex
      long unsigned int id() {
          boost::mutex::scoped_lock scoped_lock(m_mutex);
          ++m_id;
        return m_id;
      };
    private:
      /// disabled because class is singleton
      Session(const Session &);

      /// disabled because class is singleton
      Session& operator=(const Session &);

      boost::mutex m_mutex;
      unsigned long int m_id;
      
    };


  
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
