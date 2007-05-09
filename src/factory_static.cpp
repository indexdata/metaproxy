/* $Id: factory_static.cpp,v 1.18 2007-05-09 21:23:09 adam Exp $
   Copyright (c) 2005-2007, Index Data.

This file is part of Metaproxy.

Metaproxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Metaproxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Metaproxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
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
#include "filter_bounce.hpp"
#include "filter_cql_to_rpn.hpp"
#include "filter_frontend_net.hpp"
#include "filter_http_file.hpp"
#include "filter_load_balance.hpp"
#include "filter_log.hpp"
#include "filter_multi.hpp"
#include "filter_query_rewrite.hpp"
#include "filter_record_transform.hpp"
#include "filter_session_shared.hpp"
#include "filter_sru_to_z3950.hpp"
#include "filter_template.hpp"
#include "filter_virt_db.hpp"
#include "filter_z3950_client.hpp"
#include "filter_zeerex_explain.hpp"

namespace mp = metaproxy_1;

mp::FactoryStatic::FactoryStatic()
{
    struct metaproxy_1_filter_struct *buildins[] = {
        &metaproxy_1_filter_auth_simple,
        &metaproxy_1_filter_backend_test,
        &metaproxy_1_filter_bounce,
        &metaproxy_1_filter_cql_to_rpn,
        &metaproxy_1_filter_frontend_net,        
        &metaproxy_1_filter_http_file,
        &metaproxy_1_filter_load_balance,
        &metaproxy_1_filter_log,
        &metaproxy_1_filter_multi,
        &metaproxy_1_filter_query_rewrite,
        &metaproxy_1_filter_record_transform,
        &metaproxy_1_filter_session_shared,
        &metaproxy_1_filter_sru_to_z3950,
        &metaproxy_1_filter_template,
        &metaproxy_1_filter_virt_db,
        &metaproxy_1_filter_z3950_client,
        &metaproxy_1_filter_zeerex_explain,
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
