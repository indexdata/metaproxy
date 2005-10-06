/* $Id: p2_msg.cpp,v 1.2 2005-10-06 19:33:58 adam Exp $
   Copyright (c) 1998-2005, Index Data.

This file is part of the yaz-proxy.

YAZ proxy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

YAZ proxy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with YAZ proxy; see the file LICENSE.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
 */

#include <yaz/log.h>
#include <yaz/diagbib1.h>
#include "p2_backend.h"
#include "p2_frontend.h"
#include "p2_config.h"
#include "p2_modules.h"

using namespace yazpp_1;
using namespace std;

IP2_BackendSet::~IP2_BackendSet()
{
}

IP2_Backend::~IP2_Backend()
{

}

P2_Backend::P2_Backend(P2_ConfigTarget *cfg, IP2_Backend *backend_int)
{
    m_configTarget = new P2_ConfigTarget;
    *m_configTarget = *cfg;
    m_busy = false;
    m_int = backend_int;
}

P2_Backend::~P2_Backend()
{
    delete m_configTarget;
}

P2_BackendResultSet::P2_BackendResultSet()
{
    m_int = 0;
}

P2_BackendResultSet::~P2_BackendResultSet()
{
    delete m_int;
}

P2_Backend *P2_Msg::select_backend(string db,
                                   Yaz_Z_Query *query,
                                   P2_BackendResultSet **bset)
{
    P2_Config *cfg = m_server->lockConfig();

    // see if some target has done this query before

    *bset = 0;
    P2_Backend *backend = 0;

    list<P2_Backend *>::const_iterator it;
    for (it = m_server->m_backend_list.begin(); 
         it != m_server->m_backend_list.end(); it++)
    {
        if ((*it)->m_busy)
            continue;

        if (db != (*it)->m_configTarget->m_virt_database)
            continue;
        backend = *it;

        if (query)
        {
            list<P2_BackendResultSet *>::const_iterator is;
            for (is  = (*it)->m_resultSets.begin(); 
                 is != (*it)->m_resultSets.end(); is++)
            {
                if (query->match(&(*is)->m_query))
                {
                    *bset = *is;
                    break;
                }
            }
        }
        if (bset)
            break;
    }
    if (!backend)
    {
        P2_ConfigTarget *target_cfg = cfg->find_target(db);

        if (!target_cfg)
        {
            yaz_log(YLOG_WARN, "No backend for database %s",
                    db.c_str());
        }
        else
        {
            P2_ModuleInterface0 *int0 =
            reinterpret_cast<P2_ModuleInterface0 *>
                (m_server->m_modules->get_interface(target_cfg->m_type.c_str(),
                                                    0));
            IP2_Backend *bint = 0;

            if (int0)
                bint = int0->create(target_cfg->m_target_address.c_str());

            if (bint)
                backend = new P2_Backend(target_cfg, bint);

            if (backend)
                m_server->m_backend_list.push_back(backend);
        }
    }
    if (backend)
        backend->m_busy = true;
    m_server->unlockConfig();
    return backend;
}

void P2_FrontResultSet::setQuery(Z_Query *z_query)
{
    m_query.set_Z_Query(z_query);
}

void P2_FrontResultSet::setDatabases(char **db, int num)
{
    m_db_list.clear();

    int i;
    for (i = 0; i<num; i++)
        m_db_list.push_back(db[i]);
}

P2_FrontResultSet::P2_FrontResultSet(const char *id)
{
    m_resultSetId = id;
}


P2_FrontResultSet::~P2_FrontResultSet()
{
}

P2_Msg::P2_Msg(GDU *gdu, P2_Frontend *front, P2_Server *server)
{
    m_front = front;
    m_server = server;
    m_output = 0;
    m_gdu = gdu;
    m_close_flag = 0;
}

P2_Msg::~P2_Msg()
{
    delete m_output;
    delete m_gdu;
}

Z_APDU *P2_Msg::frontend_search_resultset(Z_APDU *z_gdu, ODR odr,
                                          P2_FrontResultSet **rset)
{
    Z_SearchRequest *req = z_gdu->u.searchRequest;
    list<P2_FrontResultSet *>::iterator it;
    P2_FrontResultSet *s = 0;

    string id = req->resultSetName;
    for (it = m_front->m_resultSets.begin(); it != m_front->m_resultSets.end(); it++)
    {
	if ((*it)->m_resultSetId == id)
        {
            s = *it;
	    break;
        }
    }
    if (s)
    {
	// result set already exists
        *rset = s;
	if (req->replaceIndicator && *req->replaceIndicator)
	{  // replace indicator true
	    s->setQuery(req->query);
	    s->setDatabases(req->databaseNames, req->num_databaseNames);
	    return 0;
	}
	Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
	Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
	apdu->u.searchResponse->records = rec;
	rec->which = Z_Records_NSD;
	rec->u.nonSurrogateDiagnostic =
	    zget_DefaultDiagFormat(
		odr, YAZ_BIB1_RESULT_SET_EXISTS_AND_REPLACE_INDICATOR_OFF,
		req->resultSetName);
	
	return apdu;
    }
    // does not exist 
    s = new P2_FrontResultSet(req->resultSetName);
    s->setQuery(req->query);
    s->setDatabases(req->databaseNames, req->num_databaseNames);
    m_front->m_resultSets.push_back(s);
    *rset = s;
    return 0;
}

Z_APDU *P2_Msg::frontend_search_apdu(Z_APDU *request_apdu, ODR odr)
{
    P2_FrontResultSet *rset;
    Z_APDU *response_apdu = frontend_search_resultset(request_apdu, odr,
                                                      &rset);
    if (response_apdu)
        return response_apdu;

    // no immediate error (yet) 
    size_t i;
    for (i = 0; i<rset->m_db_list.size(); i++)
    {
        string db = rset->m_db_list[i];
        P2_BackendResultSet *bset;
        P2_Backend *b = select_backend(db, &rset->m_query, &bset);
        if (!b)
        {
            Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
            Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
            apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(
                    odr, YAZ_BIB1_DATABASE_UNAVAILABLE, db.c_str());
            return apdu;
        }
        if (!bset)
        {   // new set 
            bset = new P2_BackendResultSet();

            bset->m_query.set_Z_Query(request_apdu->u.searchRequest->query);
            bset->m_db_list.push_back(db);

            b->m_int->search(&bset->m_query, &bset->m_int, &bset->m_hit_count);
            b->m_resultSets.push_back(bset);
        }
        else
        {
            bset->m_int->get(1, 1);
        }
        response_apdu = zget_APDU(odr, Z_APDU_searchResponse);
        *response_apdu->u.searchResponse->resultCount = bset->m_hit_count;
        b->m_busy = false;
    }
    if (!response_apdu)
    {
        Z_APDU *apdu = zget_APDU(odr, Z_APDU_searchResponse);
        Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
        apdu->u.searchResponse->records = rec;
            rec->which = Z_Records_NSD;
            rec->u.nonSurrogateDiagnostic =
                zget_DefaultDiagFormat(odr, YAZ_BIB1_UNSUPP_SEARCH, 0);
            return apdu;
    }
    return response_apdu;
}

Z_APDU *P2_Msg::frontend_present_resultset(Z_APDU *z_gdu, ODR odr,
                                           P2_FrontResultSet **rset)
{
    Z_PresentRequest *req = z_gdu->u.presentRequest;
    list<P2_FrontResultSet *>::iterator it;
    P2_FrontResultSet *s = 0;

    string id = req->resultSetId;
    for (it = m_front->m_resultSets.begin(); it != m_front->m_resultSets.end(); it++)
    {
	if ((*it)->m_resultSetId == id)
        {
            s = *it;
	    break;
        }
    }
    *rset = s;
    if (s)
	return 0;  // fine result set exists 

    Z_APDU *apdu = zget_APDU(odr, Z_APDU_presentResponse);
    
    Z_Records *rec = (Z_Records *) odr_malloc(odr, sizeof(Z_Records));
    apdu->u.presentResponse->records = rec;
    rec->which = Z_Records_NSD;
    rec->u.nonSurrogateDiagnostic =
	zget_DefaultDiagFormat(
	    odr,
	    YAZ_BIB1_SPECIFIED_RESULT_SET_DOES_NOT_EXIST,
	    req->resultSetId);
    return apdu;
}

Z_APDU *P2_Msg::frontend_present_apdu(Z_APDU *request_apdu, ODR odr)
{
    P2_FrontResultSet *rset;
    Z_APDU *response_apdu = frontend_present_resultset(request_apdu, odr,
                                                       &rset);
    if (response_apdu)
        return response_apdu;
    return zget_APDU(odr, Z_APDU_presentResponse);
}
    
IThreadPoolMsg *P2_Msg::handle()
{
    ODR odr = odr_createmem(ODR_ENCODE);
    yaz_log(YLOG_LOG, "P2_Msg:handle begin");
    Z_GDU *request_gdu = m_gdu->get();

    if (request_gdu->which == Z_GDU_Z3950)
    {
	Z_APDU *request_apdu = request_gdu->u.z3950;
        Z_APDU *response_apdu = 0;
        switch(request_apdu->which)
        {
        case Z_APDU_initRequest:
            response_apdu = zget_APDU(odr, Z_APDU_initResponse);
            ODR_MASK_SET(response_apdu->u.initResponse->options, Z_Options_triggerResourceCtrl);
            ODR_MASK_SET(response_apdu->u.initResponse->options, Z_Options_search);
            ODR_MASK_SET(response_apdu->u.initResponse->options, Z_Options_present);
	    ODR_MASK_SET(response_apdu->u.initResponse->options, Z_Options_namedResultSets);
            break;
        case Z_APDU_searchRequest:
            response_apdu = frontend_search_apdu(request_apdu, odr);
            break;
	case Z_APDU_presentRequest:
	    response_apdu = frontend_present_apdu(request_apdu, odr);
            break;
        case Z_APDU_triggerResourceControlRequest:
            break;
        default:
            response_apdu = zget_APDU(odr, Z_APDU_close);
            m_close_flag = 1;
            break;
        }
        if (response_apdu)
            m_output = new GDU(response_apdu);
    }
    yaz_log(YLOG_LOG, "P2_Msg:handle end");
    odr_destroy(odr);
    return this;
}

void P2_Msg::result()
{
    m_front->m_no_requests--;
    if (!m_front->m_delete_flag)
    {
        if (m_output)
        {
            int len;
            m_front->send_GDU(m_output->get(), &len);
        }
        if (m_close_flag)
        {
            m_front->close();
            m_front->m_delete_flag = 1;
        }
    }
    if (m_front->m_delete_flag && m_front->m_no_requests == 0)
        delete m_front;
    delete this;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
