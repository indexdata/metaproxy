/* $Id: factory_static.cpp,v 1.9 2006-03-16 10:40:59 adam Exp $
   Copyright (c) 2005-2006, Index Data.

%LICENSE%
*/

#include <iostream>
#include <stdexcept>

#include "factory_static.hpp"

#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"

#include "factory_filter.hpp"

#include "filter_auth_simple.hpp"
#include "filter_backend_test.hpp"
#include "filter_frontend_net.hpp"
#include "filter_http_file.hpp"
#include "filter_log.hpp"
#include "filter_multi.hpp"
#include "filter_query_rewrite.hpp"
#include "filter_session_shared.hpp"
#include "filter_template.hpp"
#include "filter_virt_db.hpp"
#include "filter_z3950_client.hpp"

namespace mp = metaproxy_1;

mp::FactoryStatic::FactoryStatic()
{
    struct metaproxy_1_filter_struct *buildins[] = {
        &metaproxy_1_filter_auth_simple,
        &metaproxy_1_filter_backend_test,
        &metaproxy_1_filter_frontend_net,        
        &metaproxy_1_filter_http_file,
        &metaproxy_1_filter_log,
        &metaproxy_1_filter_multi,
        &metaproxy_1_filter_query_rewrite,
        &metaproxy_1_filter_session_shared,
        &metaproxy_1_filter_template,
        &metaproxy_1_filter_virt_db,
        &metaproxy_1_filter_z3950_client,
        0
    };
    int i;

    for (i = 0; buildins[i]; i++)
        add_creator(buildins[i]->type, buildins[i]->creator);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
