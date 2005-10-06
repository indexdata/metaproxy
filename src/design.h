
#ifndef DESIGN_H
#define DESIGN_H

#include <stdexcept>
#include <list>

namespace yp2 {

    class Package;
    
    class Filter {
    public:
        virtual ~Filter(){};
        virtual  Package & process(Package & package) const {
            return package;
        };
        virtual  void configure(){};
        
        // set/get the C++ way .. just as showoff
        
        // get function - returns copy and keeps object const, 
        // thus is right val in assignment
        std::string name() const {
            return m_name;
        }
        // set function - returns reference and changes object,
        // thus is left val in assignment
        std::string & name() {
            return m_name;
        }
        // more traditional set function, taking const reference 
        // or copy (here const ref for demo), returning ref to object
        // can be chained with other similar functions!
        Filter & name(const std::string & name){
            m_name = name;
            return *this;
        }
        
    private:
        std::string m_name;
    };
    
    
    class Filter_Exception : public std::runtime_error {
    public:
        Filter_Exception(const std::string message)
            : std::runtime_error("Filter_Exception: " + message){
        };
    };
    

  class Router {
  public:
      Router(){};
      virtual ~Router(){};
      virtual const Filter * 
          move(const Filter *filter, const Package *package) const {
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
              //throw Router_Exception("no routing rules known");
              return 0;
          }
          return *it;
      };
      virtual void configure(){};
      Router & rule(const Filter &filter){
          m_filter_list.push_back(&filter);
      return *this;
      }
  private:
      Router(const Router &);
      Router& operator=(const Router &);
      std::list<const Filter *> m_filter_list;
  };
  
  
  class Router_Exception : public std::runtime_error {
  public:
      Router_Exception(const std::string message)
          : std::runtime_error("Router_Exception: " + message){};
  };
  
  
  class Package {
  public:
      
      // send package to it's next filter defined in chain
      Package & move() {
          m_filter = m_router->move(m_filter, this);
          if (m_filter)
              return m_filter->process(*this);
          else
              return *this;
          }
      
      // get function - returns copy and keeps object const, 
      // thus is right val in assignment
      unsigned int data() const {
          return m_data;
      }
      // set function - returns reference and changes object,
      // thus is left val in assignment
      unsigned int & data() {
          return m_data;
      }
      
      // more traditional set function, taking const reference 
      // or copy (here const ref for demo), returning ref to object
      // can be chained with other similar functions!
      Package & data(const unsigned int & data){
          m_data = data;
          return *this;
      }
      
      // get function - returns copy and keeps object const, 
      // thus is right val in assignment
      //Router router() const {
      //  return m_router;
      //}
      // set function - returns reference and changes object,
      // thus is left val in assignment
      //Router & router() {
      //  return m_router;
      //}
      // more traditional set function, taking const reference 
      // or copy (here const ref for demo), returning ref to object
      // can be chained with other similar functions!
      Package & router(const Router &router){
          m_filter = 0;
          m_router = &router;
          return *this;
      }
      Package() {
          m_filter = 0;
          m_router = 0;
          m_data = 0;
      }
      
  private:
      unsigned int m_data;
      const Filter *m_filter;
      const Router *m_router;
  };
  
  
  class Package_Exception : public std::runtime_error {
  public:
      Package_Exception(const std::string message)
          : std::runtime_error("Package_Exception: " + message){
      };
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
