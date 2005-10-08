
#include "config.hpp"
#include <dlfcn.h>

#include "p2_modules.h"

class P2_ModuleDLEntry {
public:
    void *m_dl_handle;
    P2_ModuleEntry *m_entry;
    P2_ModuleDLEntry();
    ~P2_ModuleDLEntry();
};

P2_ModuleDLEntry::P2_ModuleDLEntry()
{
    m_dl_handle = 0;
    m_entry = 0;
}
    
P2_ModuleDLEntry::~P2_ModuleDLEntry()
{
    if (m_dl_handle)
	dlclose(m_dl_handle);
}
    
P2_ModuleFactory::P2_ModuleFactory()
{
}

P2_ModuleFactory::~P2_ModuleFactory()
{
}

bool P2_ModuleFactory::add(P2_ModuleEntry *entry)
{
    P2_ModuleDLEntry *m = new P2_ModuleDLEntry();
    m->m_entry = entry;
    m_modules.push_back(m);
    return true;
}

bool P2_ModuleFactory::add(const char *fname)
{
    void *dl_handle = dlopen(fname, RTLD_NOW|RTLD_GLOBAL);
    if (!dl_handle)
	return false;

    P2_ModuleEntry *entry =
	reinterpret_cast<P2_ModuleEntry *> 
	(dlsym(dl_handle, "p2_module_entry"));
    if (!entry)
    {
	dlclose(dl_handle);
	return false;
    }
    P2_ModuleDLEntry *m = new P2_ModuleDLEntry();
    m->m_dl_handle = dl_handle;
    m->m_entry = entry;
    m_modules.push_back(m);
    return true;
}

void *P2_ModuleFactory::get_interface(const char *name, int version)
{
    std::list<P2_ModuleDLEntry *>::const_iterator it;
    for (it = m_modules.begin();  it != m_modules.end(); it++)
    {
	P2_ModuleDLEntry *ent = *it;
	if (!strcmp(ent->m_entry->name, name) &&
	    ent->m_entry->version == version)
	{
	    return ent->m_entry->interface_ptr;
	}
    }
    return 0;
}

