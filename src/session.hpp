/* $Id: session.hpp,v 1.10 2005-10-25 21:32:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef SESSION_HPP
#define SESSION_HPP

//#include <stdexcept>
#include <map>
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

        bool operator<(const Session &s) const {
            return m_id < s.m_id ? true : false;
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

        bool operator == (Session &ses) const {
            return ses.m_id == m_id;
        }
    private:
        
        unsigned long int m_id;
        bool m_close;
        
        /// static mutex to lock static m_id
        static boost::mutex m_mutex;
        
        /// static m_id to make sure that there is only one id counter
        static unsigned long int m_global_id;
        
    };

    template <class T> class session_map {
    public:
        void create(T &t, const yp2::Session &s) { 
            boost::mutex::scoped_lock lock(m_mutex);
            m_map[s] = t;
        };
        void release(const yp2::Session &s) {
            boost::mutex::scoped_lock lock(m_mutex);

            m_map.erase(s);
        };
        bool active(const yp2::Session &s) {
            typename std::map<yp2::Session,T>::const_iterator it;
            it = m_map.find(s);
            return it == m_map.end() ? false : true;
        }
    private:
        boost::mutex m_mutex;
        std::map<yp2::Session,T>m_map;
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
