/* This file is part of Metaproxy.
   Copyright (C) 2005-2011 Index Data

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

#include <iostream>
#include <stdexcept>

#include "factory_static.hpp"

#include "config.hpp"
#include <metaproxy/filter.hpp>
#include <metaproxy/package.hpp>

#include "factory_filter.hpp"

#include "filter_auth_simple.hpp"
#include "filter_backend_test.hpp"
#include "filter_bounce.hpp"
#ifndef WIN32
#include "filter_cgi.hpp"
#endif
#include "filter_cql_to_rpn.hpp"
#include "filter_frontend_net.hpp"
#include "filter_http_file.hpp"
#include "filter_limit.hpp"
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
#ifndef WIN32
        &metaproxy_1_filter_cgi,
#endif
        &metaproxy_1_filter_cql_to_rpn,
        &metaproxy_1_filter_frontend_net,        
        &metaproxy_1_filter_http_file,
        &metaproxy_1_filter_limit,
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
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

