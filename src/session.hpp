/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

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

#ifndef SESSION_HPP
#define SESSION_HPP

//#include <stdexcept>
#include <map>
#include <boost/thread/mutex.hpp>

namespace metaproxy_1 {
    
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
        void create(T &t, const metaproxy_1::Session &s) { 
            boost::mutex::scoped_lock lock(m_map_mutex);
            m_map[s] = SessionItem(t);
        };
        void release(const metaproxy_1::Session &s) {
            boost::mutex::scoped_lock lock(m_map_mutex);

            m_map.erase(s);
        };
#if 0
        T &get_session_data(const metaproxy_1::Session &s) {
            boost::mutex::scoped_lock lock(m_map_mutex);

            typename std::map<metaproxy_1::Session,SessionItem>::const_iterator it;
            it = m_map.find(s);
            if (it == m_map.end())
                return 0;
            boost::mutx::scoped_lock *scoped_ptr =
                new boost::mutex::scoped_lock(it->second->m_item_mutex);
        };
#endif
        bool exist(const metaproxy_1::Session &s) {
            typename std::map<metaproxy_1::Session,SessionItem>::const_iterator it;
            it = m_map.find(s);
            return it == m_map.end() ? false : true;
        }
    private:
        class SessionItem {
        public:
            SessionItem() {};
            SessionItem(T &t) : m_t(t) {};
            SessionItem &operator =(const SessionItem &s) {
                if (this != &s) {
                    m_t = s.m_t;
                }
                return *this;
            };
            SessionItem(const SessionItem &s) {
                m_t = s.m_t;
            };
            T m_t;
            boost::mutex m_item_mutex;
        };
    private:
        boost::mutex m_map_mutex;
        std::map<metaproxy_1::Session,SessionItem>m_map;
    };
    
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

