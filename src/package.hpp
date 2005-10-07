
#ifndef PACKAGE_HPP
#define PACKAGE_HPP

#include <stdexcept>


namespace yp2 {
  
  class Package {
  public:
      
      Package(unsigned long int id = 0, bool close = 0) 
          : m_session_id(id),  m_session_close(close),
          m_filter(0), m_router(0), m_data(0)  {}

      /// send Package to it's next Filter defined in Router
      Package & move() {
          m_filter = m_router->move(m_filter, this);
          if (m_filter)
              return m_filter->process(*this);
          else
              return *this;
          }
      

      /// get function - right val in assignment
      unsigned int session_id() const {
          return m_session_id;
      }
      
      /// get function - right val in assignment
      unsigned int session_close() const {
          return m_session_close;
      }
   

      /// get function - right val in assignment
      unsigned int data() const {
          return m_data;
      }

      /// set function - left val in assignment
      unsigned int & data() {
          return m_data;
      }

      /// set function - can be chained
      Package & data(const unsigned int & data){
          m_data = data;
          return *this;
      }
      

      //Router router() const {
      //  return m_router;
      //}

      //Router & router() {
      //  return m_router;
      //}

      /// set function - can be chained
      Package & router(const Router &router){
          m_filter = 0;
          m_router = &router;
          return *this;
      }

      
  private:
      unsigned long int m_session_id;
      bool m_session_close;
      const Filter *m_filter;
      const Router *m_router;
      unsigned int m_data;
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
