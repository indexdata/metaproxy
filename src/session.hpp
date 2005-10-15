/* $Id: session.hpp,v 1.7 2005-10-15 14:09:09 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

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
        
        Session& operator=(const Session &s) {
            if (this != &s)
            {
                m_id = s.m_id;
                m_close = s.m_close;
            }
            return *this;
        }
        
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

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
