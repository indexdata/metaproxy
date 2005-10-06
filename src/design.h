
#ifndef DESIGN_H
#define DESIGN_H

#include <stdexcept>


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
      route(const Filter *filter, const Package *package) const {
      //if (!m_sillyrule)
      //throw Router_Exception("no routing rules known");
          return m_filter;
    };
    virtual void configure(){};
    Router & rule(const Filter &filter){
      m_filter = &filter;
      return *this;
    }
  private:
    Router(const Router &);
    Router& operator=(const Router &);
    const Filter *m_filter;
  };
  
  
  class Router_Exception : public std::runtime_error {
  public:
    Router_Exception(const std::string message)
      : std::runtime_error("Router_Exception: " + message){};
  };
  

  class Package {
  public:

    // send package to it's next filter defined in chain
    void move() 
      {
        m_filter = m_router->route(m_filter, this);
        if (m_filter)
          m_filter->process(*this);
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
      m_router = &router;
      return *this;
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
