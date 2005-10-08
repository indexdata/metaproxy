#include "config.hpp"
#include <yaz/log.h>
#include "p2_backend.h"

class P2_BackendSetDummy : public IP2_BackendSet {
public:
    P2_BackendSetDummy();
    ~P2_BackendSetDummy();
    int get(int start, int number);
};

class P2_BackendDummy : public IP2_Backend {
public:
    P2_BackendDummy(const char *address);
    ~P2_BackendDummy();
    int search(yazpp_1::Yaz_Z_Query *q, IP2_BackendSet **rset, int *hits);
};

P2_BackendDummy::P2_BackendDummy(const char *address)
{
    yaz_log(YLOG_LOG, "P2_backendDummy %p create", this);
}

P2_BackendDummy::~P2_BackendDummy()
{
    yaz_log(YLOG_LOG, "P2_backendDummy %p destroy", this);
}

int P2_BackendDummy::search(yazpp_1::Yaz_Z_Query *q, IP2_BackendSet **rset,
			    int *hits)
{
    yaz_log(YLOG_LOG, "P2_backendDummy %p search", this);

    P2_BackendSetDummy *s = new P2_BackendSetDummy();

    *rset = s;
    *hits = 42;
    return 0;
}

int P2_BackendSetDummy::get(int start, int number)
{
    yaz_log(YLOG_LOG, "P2_backendSetDummy %p get", this);
    return 0;
}

P2_BackendSetDummy::P2_BackendSetDummy()
{
    yaz_log(YLOG_LOG, "P2_backendSetDummy %p create", this);

}

P2_BackendSetDummy::~P2_BackendSetDummy()
{
    yaz_log(YLOG_LOG, "P2_backendSetDummy %p destroy", this);
}

static IP2_Backend *dummy_create(const char *address)
{
    return new P2_BackendDummy(address);
}

P2_ModuleInterface0 int0 = {
    dummy_create
};

P2_ModuleEntry p2_module_entry = {
    0,
    "dummy",
    "Dummy Backend",
    (void *) &int0
};

P2_ModuleEntry *p2_backend_dummy = &p2_module_entry;
