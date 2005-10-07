
#ifndef FILTER_HPP
#define FILTER_HPP

#include <stdexcept>

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

  
}

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
