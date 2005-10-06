
#ifndef P3_FILTER_H
#define P3_FILTER_H

#include <stdexcept>


namespace p3 {

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
    unsigned int max_front_workers() const {
      return m_max_front;
    }
    // set function - returns reference and changes object,
    // thus is left val in assignment
    unsigned int & max_front_workers() {
      return m_max_front;
    }
    // more traditional set function, taking const reference 
    // or copy (here const ref for demo), returning ref to object
    // can be chained with other similar functions!
    Filter & max_front_workers(const unsigned int & max_front){
      m_max_front = max_front;
      return *this;
    }
    
  private:
    unsigned int m_max_front;
  };


  class Filter_Exception : public std::runtime_error {
  public:
    Filter_Exception(const std::string message)
      : std::runtime_error("Filter_Exception: " + message){
    };
  };


  class Router {
  public:
    virtual ~Router(){};
    virtual const Filter & 
      route(const Filter & filter, Package & package) const {
      //if (!m_sillyrule)
      //throw Router_Exception("no routing rules known");
          return m_sillyrule;
    };
    virtual void configure(){};
    Router & rule(Filter filter){
      m_sillyrule = filter;
      return *this;
    }
  private:
    Filter m_sillyrule;
  };
  
  
  class Router_Exception : public std::runtime_error {
  public:
    Router_Exception(const std::string message)
      : std::runtime_error("Router_Exception: " + message){};
  };
  

  class Package {
  public:

    // send package to it's next filter defined in chain
    void move() {
      Filter oldfilter;
      Filter nextfilter = m_router.route(oldfilter, *this);
      nextfilter.process(*this);
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
    Router router() const {
      return m_router;
    }
    // set function - returns reference and changes object,
    // thus is left val in assignment
    Router & router() {
      return m_router;
    }
    // more traditional set function, taking const reference 
    // or copy (here const ref for demo), returning ref to object
    // can be chained with other similar functions!
    Package & router(const Router & router){
      m_router = router;
      return *this;
    }
    
  private:
    unsigned int m_data;
    Router m_router;
  };


  class Package_Exception : public std::runtime_error {
  public:
    Package_Exception(const std::string message)
      : std::runtime_error("Package_Exception: " + message){
    };
  };



  
}

#endif
