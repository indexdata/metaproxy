/* This file is part of Metaproxy.
   Copyright (C) 2005-2008 Index Data

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

namespace mp = metaproxy_1;

namespace metaproxy_1 {
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

mp::FactoryFilter::NotFound::NotFound(const std::string message)
    : std::runtime_error(message)
{
}

mp::FactoryFilter::Rep::Rep()
{
}

mp::FactoryFilter::Rep::~Rep()
{
}

mp::FactoryFilter::FactoryFilter() : m_p(new mp::FactoryFilter::Rep)
{

}

mp::FactoryFilter::~FactoryFilter()
{

}

bool mp::FactoryFilter::add_creator(std::string fi,
                                     CreateFilterCallback cfc)
{
    return m_p->m_fcm.insert(Rep::CallbackMap::value_type(fi, cfc)).second;
}


bool mp::FactoryFilter::drop_creator(std::string fi)
{
    return m_p->m_fcm.erase(fi) == 1;
}

bool mp::FactoryFilter::exist(std::string fi)
{
    Rep::CallbackMap::const_iterator it = m_p->m_fcm.find(fi);
    
    if (it == m_p->m_fcm.end())
    {
        return false;
    }
    return true;
}

mp::filter::Base* mp::FactoryFilter::create(std::string fi)
{
    Rep::CallbackMap::const_iterator it = m_p->m_fcm.find(fi);
    
    if (it == m_p->m_fcm.end()){
        std::string msg = "filter type '" + fi + "' not found";
            throw NotFound(msg);
    }
    // call create function
    return (it->second());
}

bool mp::FactoryFilter::have_dl_support()
{
#if HAVE_DLFCN_H
    return true;
#else
    return false;
#endif
}

bool mp::FactoryFilter::add_creator_dl(const std::string &fi,
                                        const std::string &path)
{
#if HAVE_DLFCN_H
    if (m_p->m_fcm.find(fi) != m_p->m_fcm.end())
    {
        return true;
    }

    std::string full_path = path + "/metaproxy_filter_" + fi + ".so";
    void *dl_handle = dlopen(full_path.c_str(), RTLD_GLOBAL|RTLD_NOW);
    if (!dl_handle)
    {
        const char *dl = dlerror();
        std::cout << "dlopen " << full_path << " failed. dlerror=" << dl << 
            std::endl;
        return false;
    }

    std::string full_name = "metaproxy_1_filter_" + fi;
    
    void *dlsym_ptr = dlsym(dl_handle, full_name.c_str());
    if (!dlsym_ptr)
    {
        std::cout << "dlsym " << full_name << " failed\n";
        return false;
    }
    struct metaproxy_1_filter_struct *s = (struct metaproxy_1_filter_struct *) dlsym_ptr;
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
