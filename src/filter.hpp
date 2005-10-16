/* $Id: filter.hpp,v 1.5 2005-10-16 16:05:44 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#ifndef FILTER_HPP
#define FILTER_HPP

#include <stdexcept>

namespace yp2 {

    class Package;

    namespace filter {
        class Base {
        public:
            virtual ~Base(){};
            
            ///sends Package off to next Filter, returns altered Package
            virtual void process(Package & package) const = 0;

            virtual void configure(){};
            
            /// get function - right val in assignment
            std::string name() const {
                return m_name;
            }
            
            /// set function - left val in assignment
            std::string & name() {
                return m_name;
            }
            
            /// set function - can be chained
            Base & name(const std::string & name){
                m_name = name;
                return *this;
            }
            
        private:
            std::string m_name;
        };
    }
    
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
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
