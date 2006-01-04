/* $Id: factory_static.cpp,v 1.2 2006-01-04 11:55:31 adam Exp $
   Copyright (c) 2005, Index Data.

%LICENSE%

*/

#include <iostream>
#include <stdexcept>

#include "factory_static.hpp"

#include "config.hpp"
#include "filter.hpp"
#include "package.hpp"

#include "filter_factory.hpp"

#include "filter_backend_test.hpp"
#include "filter_frontend_net.hpp"
#include "filter_log.hpp"
#include "filter_session_shared.hpp"
#include "filter_template.hpp"
#include "filter_virt_db.hpp"
#include "filter_z3950_client.hpp"

yp2::FactoryStatic::FactoryStatic(yp2::FilterFactory &factory)
{
    struct yp2_filter_struct *buildins[] = {
        &yp2_filter_backend_test,
        &yp2_filter_frontend_net,        
        &yp2_filter_log,
        &yp2_filter_session_shared,
        &yp2_filter_template,
        &yp2_filter_virt_db,
        &yp2_filter_z3950_client,
        0
    };
    int i;

    for (i = 0; buildins[i]; i++)
        factory.add_creator(buildins[i]->type, buildins[i]->creator);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * c-file-style: "stroustrup"
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
