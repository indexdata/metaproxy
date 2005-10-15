
#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <stdexcept>
#include <list>

namespace yp2 {
    namespace filter {
        class Base;
    }
    class Package;
    
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
        virtual const filter::Base *move(const filter::Base *filter,
                                   const Package *package) const {
            return 0;
        };

        /// re-read configuration of routing tables
        virtual void configure(){};

        /// add routing rule expressed as Filter to Router
        virtual Router & rule(const filter::Base &filter){
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
        virtual const filter::Base *move(const filter::Base *filter,
                                   const Package *package) const {
            std::list<const filter::Base *>::const_iterator it;
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
        RouterChain & rule(const filter::Base &filter){
            m_filter_list.push_back(&filter);
            return *this;
        }
    protected:
        std::list<const filter::Base *> m_filter_list;
    private:
        /// disabled because class is singleton
        RouterChain(const RouterChain &);

        /// disabled because class is singleton
        RouterChain& operator=(const RouterChain &);
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
