
#ifndef P2_MODULES_H
#define P2_MODULES_H

#include "p2_backend.h"

#include <list>

class P2_ModuleDLEntry ;
class P2_ModuleFactory {
 public:
    P2_ModuleFactory();
    ~P2_ModuleFactory();
    bool add(const char *fname);
    bool add(P2_ModuleEntry *entry);
    void *get_interface(const char *name, int version);
 private:
    std::list <P2_ModuleDLEntry *>m_modules;
};

#endif
