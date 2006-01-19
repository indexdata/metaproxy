/* $Id: factory_filter.cpp,v 1.3 2006-01-19 09:41:01 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%
 */

#include "config.hpp"

#include "factory_filter.hpp"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <stdexcept>
#include <iostream>
#include <string>
#include <map>

namespace yp2 {
    class FactoryFilter::Rep {
        typedef std::map<std::string, CreateFilterCallback> CallbackMap;
        typedef std::map<std::string, CreateFilterCallback>::iterator 
            CallbackMapIt;
    public:
        friend class FactoryFilter;
        CallbackMap m_fcm;
        Rep();
        ~Rep();
    };
}

yp2::FactoryFilter::NotFound::NotFound(const std::string message)
    : std::runtime_error(message)
{
}

yp2::FactoryFilter::Rep::Rep()
{
}

yp2::FactoryFilter::Rep::~Rep()
{
}

yp2::FactoryFilter::FactoryFilter() : m_p(new yp2::FactoryFilter::Rep)
{

}

yp2::FactoryFilter::~FactoryFilter()
{

}

bool yp2::FactoryFilter::add_creator(std::string fi,
                                     CreateFilterCallback cfc)
{
    return m_p->m_fcm.insert(Rep::CallbackMap::value_type(fi, cfc)).second;
}


bool yp2::FactoryFilter::drop_creator(std::string fi)
{
    return m_p->m_fcm.erase(fi) == 1;
}

bool yp2::FactoryFilter::exist(std::string fi)
{
    Rep::CallbackMap::const_iterator it = m_p->m_fcm.find(fi);
    
    if (it == m_p->m_fcm.end())
    {
        return false;
    }
    return true;
}

yp2::filter::Base* yp2::FactoryFilter::create(std::string fi)
{
    Rep::CallbackMap::const_iterator it = m_p->m_fcm.find(fi);
    
    if (it == m_p->m_fcm.end()){
        std::string msg = "filter type '" + fi + "' not found";
            throw NotFound(msg);
    }
    // call create function
    return (it->second());
}

bool yp2::FactoryFilter::have_dl_support()
{
#if HAVE_DLFCN_H
    return true;
#else
    return false;
#endif
}

bool yp2::FactoryFilter::add_creator_dl(const std::string &fi,
                                        const std::string &path)
{
#if HAVE_DLFCN_H
    if (m_p->m_fcm.find(fi) != m_p->m_fcm.end())
    {
        return true;
    }

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
#else
    return false;
#endif
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
