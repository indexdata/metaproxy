/* $Id: filter_factory.cpp,v 1.1 2005-11-10 23:10:42 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "filter_factory.hpp"

#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

namespace yp2 {
    class FilterFactory::Rep {
    public:
        friend class FilterFactory;
        CallbackMap m_fcm;
        Rep();
        ~Rep();
    };
}

yp2::FilterFactoryException::FilterFactoryException(const std::string message)
    : std::runtime_error("FilterException: " + message)
{
}

yp2::FilterFactory::Rep::Rep()
{
}

yp2::FilterFactory::Rep::~Rep()
{
}

yp2::FilterFactory::FilterFactory() : m_p(new yp2::FilterFactory::Rep)
{

}

yp2::FilterFactory::~FilterFactory()
{

}

bool yp2::FilterFactory::add_creator(std::string fi,
                                     CreateFilterCallback cfc)
{
    return m_p->m_fcm.insert(CallbackMap::value_type(fi, cfc)).second;
}


bool yp2::FilterFactory::drop_creator(std::string fi)
{
    return m_p->m_fcm.erase(fi) == 1;
}

yp2::filter::Base* yp2::FilterFactory::create(std::string fi)
{
    CallbackMap::const_iterator it = m_p->m_fcm.find(fi);
    
    if (it == m_p->m_fcm.end()){
        std::string msg = "filter type '" + fi + "' not found";
            throw yp2::FilterFactoryException(msg);
    }
    // call create function
    return (it->second());
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
