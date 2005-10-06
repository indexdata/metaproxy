
#ifndef P2_BACKEND_H
#define P2_BACKEND_H

#include <yaz++/z-query.h>

class IP2_BackendSet {
public:
    virtual ~IP2_BackendSet();
    virtual int get(int start, int number) = 0;
};

class IP2_Backend {
 public:
    virtual ~IP2_Backend();
    virtual int search(yazpp_1::Yaz_Z_Query *q, IP2_BackendSet **rset, int *hits) = 0;
};

struct P2_ModuleInterface0 {
    IP2_Backend *(*create)(const char *address);
};
    
struct P2_ModuleEntry {
    int version;
    const char *name;
    const char *description;
    void *interface_ptr;
};

    
#endif
