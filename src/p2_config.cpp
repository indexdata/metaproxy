/* $Id: p2_config.cpp,v 1.1 2005-10-06 09:37:25 marc Exp $
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

#include <stdlib.h>
#include <string.h>
#include <yaz/log.h>
#include <yaz/options.h>
#include <yaz/diagbib1.h>
#include "p2_config.h"

#if HAVE_XSLT
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>
#endif

#include <iostream>

using namespace std;

class P2_Config::Rep {
    
public:
    Rep();
    ~Rep();
public:
#if HAVE_XSLT
    xmlDocPtr m_docPtr;
    xmlNodePtr m_proxyPtr;
#endif
};

P2_Config::Rep::Rep()
{
#if HAVE_XSLT
    m_docPtr = 0;
    m_proxyPtr = 0;
#endif
}

P2_Config::Rep::~Rep()
{
#if HAVE_XSLT
    if (m_docPtr)
        xmlFreeDoc(m_docPtr);
#endif
}
    
P2_Config::P2_Config()
{
    m_max_clients = 500;
    m_client_idletime = 600;
    m_debug_mode = 0;
    m_no_limit_files = 0;
    m_no_threads = 20;
    m_target_idletime = 600;

    m_rep = new Rep();
}

bool P2_Config::parse_options(int argc, char **argv)
{
    char *arg;
    int ret;
    bool show_config = false;
    while ((ret = options("o:a:t:v:c:u:i:m:l:T:p:n:h:XS",
                          argv, argc, &arg)) != -2)
    {
        switch (ret)
        {
        case 0:
            if (m_listen_address.length())
            {
                yaz_log(YLOG_FATAL, "Multiple listener address given");
                return false;
            }
            m_listen_address = arg;
            break;
        case 'a':
            m_apdu_log = arg;
            break;
        case 'c':
            if (m_xml_fname.length())
            {
                yaz_log(YLOG_FATAL, "Multiple -c options given");
                return false;
            }
            if (!read_xml_config(arg))
            {
                return false;
            }
            m_xml_fname = arg;
            break;
        case 'i':
            m_client_idletime = atoi(arg);
            break;
        case 'l':
            m_log_file = arg;
            break;
        case 'm':
            m_max_clients = atoi(arg);
            break;
        case 'n':
            m_no_limit_files = atoi(arg);
            break;
        case 'h':
            m_no_threads = atoi(arg);
            break;
        case 'o':
            m_optimize_flags = arg;
            break;
        case 'p':
            if (m_pid_fname.length())
            {
                yaz_log(YLOG_LOG, "Multiple -p options given");
                return false;
            }
            m_pid_fname = arg;
            break;
        case 't':
            if (m_default_target.length())
            {
                yaz_log(YLOG_LOG, "Multiple -t options given");
                return false;
            }
            m_default_target = arg;
            break;
        case 'T':
            m_target_idletime = atoi(arg);
            break;
        case 'u':
            if (m_uid.length())
            {
                yaz_log(YLOG_FATAL, "-u specified more than once");
                return false;
            }
            m_uid = arg;
            break;
        case 'v':
            yaz_log_init_level(yaz_log_mask_str(arg));
            break;
        case 'X':
            m_debug_mode = 1;
            break;
        case 'S':
            show_config = true;
            break;
        default:
            yaz_log(YLOG_FATAL, "Bad option %s", arg);
            return false;
        }
    } 
    if (m_log_file.length())
        yaz_log_init_file(m_log_file.c_str());
    if (show_config)
        print();
    return true;
}

bool P2_Config::parse_xml_text(void *xml_ptr, bool &val)
{
    string v;
    if (!parse_xml_text(xml_ptr, v))
        return false;
    if (v.length() == 1 && v[0] == '1')
        val = true;
    else
        val = false;
    return true;
}

bool P2_Config::parse_xml_text(void *xml_ptr, string &val)
{
    xmlNodePtr ptr = (xmlNodePtr) xml_ptr;
    bool found = false;
    string v;
    for(ptr = ptr->children; ptr; ptr = ptr->next)
        if (ptr->type == XML_TEXT_NODE)
        {
            xmlChar *t = ptr->content;
            if (t)
            {
                v += (const char *) t;
                found = true;
            }
        }
    if (found)
        val = v;
    return found;
}

void P2_Config::parse_xml_element_target(void *xml_ptr,
                                         P2_ConfigTarget *t)
{
    xmlNodePtr ptr = (xmlNodePtr) xml_ptr;

    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "url"))
        {
            parse_xml_text(ptr, t->m_target_address);
        }
        else if (!strcmp((const char *) ptr->name, "database"))
        {
            parse_xml_text(ptr, t->m_target_database);
        }
        else
        {
            yaz_log(YLOG_WARN, "Unknown element '%s' inside target",
                    (const char *) ptr->name);
            m_errors++;
        }
    }
}

void P2_Config::parse_xml_element_proxy(void *xml_ptr)
{
    xmlNodePtr ptr = (xmlNodePtr) xml_ptr;

    for (ptr = ptr->children; ptr; ptr = ptr->next)
    {
        if (ptr->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) ptr->name, "target"))
        {
            P2_ConfigTarget *t = new P2_ConfigTarget();

            struct _xmlAttr *attr;
            for (attr = ptr->properties; attr; attr = attr->next)
                if (!strcmp((const char *) attr->name, "name")
                    || !strcmp((const char *) attr->name, "host"))
                {
                    parse_xml_text(attr, t->m_virt_address);
                }
                else if (!strcmp((const char *) attr->name, "database"))
                {
                    parse_xml_text(attr, t->m_virt_database);
                }
                else if (!strcmp((const char *) attr->name, "default"))
                {
                    parse_xml_text(attr, t->m_default);
                }
                else if (!strcmp((const char *) attr->name, "type"))
                {
                    parse_xml_text(attr, t->m_type);
                }
                else
                {
                    yaz_log(YLOG_WARN, "Unknown attribute '%s' for "
                            "element proxy",
                            (const char *) attr->name);
                    m_errors++;
                }
            parse_xml_element_target(ptr, t);
            m_target_list.push_back(t);
        }
        else if (!strcmp((const char *) ptr->name, "max-clients"))
        {
            string v;
            if (parse_xml_text(ptr, v))
                m_max_clients = atoi(v.c_str());
        }
        else if (!strcmp((const char *) ptr->name, "module"))
        {
            P2_ConfigModule *t = new P2_ConfigModule();

            string v;
            if (parse_xml_text(ptr, v))
            {
                t->m_fname = v;
                m_modules.push_back(t);
            }
        }
        else
        {
            yaz_log(YLOG_WARN, "Unknown element '%s' inside proxy", ptr->name);
            m_errors++;
        }
    }
}

void P2_Config::print()
{
    cout << "max_clients=" << m_max_clients << endl;
    list<P2_ConfigTarget *>::const_iterator it;
    
    for (it = m_target_list.begin(); it != m_target_list.end(); it++)
    {
        cout << "type=" << (*it)->m_type << " ";
        cout << "v-address=" << (*it)->m_virt_address << " ";
        cout << "v-db=" << (*it)->m_virt_database << " ";
        cout << "t-address=" << (*it)->m_target_address << " ";
        cout << "t-db=" << (*it)->m_target_database << " ";
        cout << "default=" << (*it)->m_default << endl;
    }
}

bool P2_Config::read_xml_config(const char *fname)
{
    xmlDocPtr ndoc = xmlParseFile(fname);

    if (!ndoc)
    {
        yaz_log(YLOG_WARN, "Config file %s not found or parse error", fname);
        return false;
    }
    int noSubstitutions = xmlXIncludeProcess(ndoc);
    if (noSubstitutions == -1)
        yaz_log(YLOG_WARN, "XInclude processing failed on config %s", fname);

    xmlNodePtr proxyPtr = xmlDocGetRootElement(ndoc);
    if (!proxyPtr || proxyPtr->type != XML_ELEMENT_NODE ||
        strcmp((const char *) proxyPtr->name, "proxy"))
    {
        yaz_log(YLOG_WARN, "No proxy element in %s", fname);
        xmlFreeDoc(ndoc);
        return false;
    }
    m_rep->m_proxyPtr = proxyPtr;
    
    // OK: release previous and make it the current one.
    if (m_rep->m_docPtr)
        xmlFreeDoc(m_rep->m_docPtr);
    m_rep->m_docPtr = ndoc;

    m_errors = 0;
    parse_xml_element_proxy(proxyPtr);
    if (m_errors && !m_debug_mode)
        return false;
    return true;
}

P2_Config::~P2_Config()
{
    delete m_rep;
}

P2_ConfigTarget::P2_ConfigTarget()
{
    m_default = false;
}

P2_ConfigTarget *P2_Config::find_target(string db)
{
    list<P2_ConfigTarget *>::const_iterator it;
    for (it = m_target_list.begin(); it != m_target_list.end(); it++)
    {
        if ((*it)->m_virt_database == db)
            return (*it);
    }
    return 0;
}


/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
