
#ifndef SESSION_HPP
#define SESSION_HPP

//#include <stdexcept>

#include <boost/thread/mutex.hpp>

namespace yp2 {

    class Session
    {
        //typedef unsigned long type;
    public:

       /// create new session with new unique id
       Session() {
            boost::mutex::scoped_lock scoped_lock(m_mutex);
            ++m_global_id;
            m_id =  m_global_id;
            m_close = false;
        };

      /// copy session including old id
      Session(const Session &s) : m_id(s.m_id), m_close(s.m_close) {};

        //Session& operator=(const Session &);
        
        unsigned long id() const {
            return m_id;
        };
        
        bool is_closed() const {
            return m_close;
        };

        /// mark session closed, can not be unset
        void close() {
            m_close = true;
        };

     private:

        unsigned long int m_id;
        bool m_close;

        /// static mutex to lock static m_id
        static boost::mutex m_mutex;
        
        /// static m_id to make sure that there is only one id counter
        static unsigned long int m_global_id;
        
    };
    
}

// defining and initializing static members
boost::mutex yp2::Session::m_mutex;
unsigned long int yp2::Session::m_global_id = 0;


#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
