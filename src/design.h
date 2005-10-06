
#ifndef DESIGN_H
#define DESIGN_H

#include <stdexcept>
#include <list>

#include <boost/thread/mutex.hpp>

namespace yp2 {

    class Package;
    
    class Filter {
    public:
        virtual ~Filter(){};

        ///sends Package off to next Filter, returns altered Package
        virtual  Package & process(Package & package) const {
            return package;
        };
        virtual  void configure(){};

        /// get function - right val in assignment
        std::string name() const {
            return m_name;
        }

        /// set function - left val in assignment
        std::string & name() {
            return m_name;
        }

        /// set function - can be chained
        Filter & name(const std::string & name){
            m_name = name;
            return *this;
        }
        
    private:
        std::string m_name;
    };
    
    
    class FilterException : public std::runtime_error {
    public:
        FilterException(const std::string message)
            : std::runtime_error("FilterException: " + message){
        };
    };
    
  
    class RouterException : public std::runtime_error {
    public:
        RouterException(const std::string message)
            : std::runtime_error("RouterException: " + message){};
    };
  
    
    class Router {
    public:
        Router(){};
        virtual ~Router(){};

        /// determines next Filter to use from current Filter and Package
        virtual const Filter *move(const Filter *filter,
                                   const Package *package) const {
            return 0;
        };

        /// re-read configuration of routing tables
        virtual void configure(){};

        /// add routing rule expressed as Filter to Router
        virtual Router & rule(const Filter &filter){
            return *this;
        }
    private:
        /// disabled because class is singleton
        Router(const Router &);

        /// disabled because class is singleton
        Router& operator=(const Router &);
    };
  
    
    class RouterChain : public Router {
    public:
        RouterChain(){};
        virtual ~RouterChain(){};
        virtual const Filter *move(const Filter *filter,
                                   const Package *package) const {
            std::list<const Filter *>::const_iterator it;
            it = m_filter_list.begin();
            if (filter)
                {
                    for (; it != m_filter_list.end(); it++)
                        if (*it == filter)
                            {
                                it++;
                                break;
                            }
                }
            if (it == m_filter_list.end())
                {
                    //throw RouterException("no routing rules known");
                    return 0;
                }
            return *it;
        };
        virtual void configure(){};
        RouterChain & rule(const Filter &filter){
            m_filter_list.push_back(&filter);
            return *this;
        }
    protected:
        std::list<const Filter *> m_filter_list;
    private:
        /// disabled because class is singleton
        RouterChain(const RouterChain &);

        /// disabled because class is singleton
        RouterChain& operator=(const RouterChain &);
    };
  

  class PackageException : public std::runtime_error {
  public:
      PackageException(const std::string message)
          : std::runtime_error("PackageException: " + message){
      };
  };
  
  
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
