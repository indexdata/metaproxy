/* $Id: filter_factory.cpp,v 1.2 2005-12-10 09:59:10 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "filter_factory.hpp"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

namespace yp2 {
    class FilterFactory::Rep {
        typedef std::map<std::string, CreateFilterCallback> CallbackMap;
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
    return m_p->m_fcm.insert(Rep::CallbackMap::value_type(fi, cfc)).second;
}


bool yp2::FilterFactory::drop_creator(std::string fi)
{
    return m_p->m_fcm.erase(fi) == 1;
}

yp2::filter::Base* yp2::FilterFactory::create(std::string fi)
{
    Rep::CallbackMap::const_iterator it = m_p->m_fcm.find(fi);
    
    if (it == m_p->m_fcm.end()){
        std::string msg = "filter type '" + fi + "' not found";
            throw yp2::FilterFactoryException(msg);
    }
    // call create function
    return (it->second());
}

#if HAVE_DLFCN_H
bool yp2::FilterFactory::add_creator_dyn(const std::string &fi,
                                         const std::string &path)
{
    std::string full_path = path + "/yp2_filter_" + fi + ".so";
    void *dl_handle = dlopen(full_path.c_str(), RTLD_GLOBAL|RTLD_NOW);
    if (!dl_handle)
    {
        const char *dl = dlerror();
        std::cout << "dlopen " << full_path << " failed. dlerror=" << dl << 
            std::endl;
        return false;
    }

    std::string full_name = "yp2_filter_" + fi;
    
    void *dlsym_ptr = dlsym(dl_handle, full_name.c_str());
    if (!dlsym_ptr)
    {
        std::cout << "dlsym " << full_name << " failed\n";
        return false;
    }
    struct yp2_filter_struct *s = (struct yp2_filter_struct *) dlsym_ptr;
    return add_creator(fi, s->creator);
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
