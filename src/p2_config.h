/* $Id: p2_config.h,v 1.1.1.1 2005-10-06 09:37:25 marc Exp $
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

#ifndef P2_CONFIG_INCLUDED
#define P2_CONFIG_INCLUDED

#include <string>
#include <list>

class P2_ConfigTarget {
 public:
    P2_ConfigTarget();
    std::string m_virt_address;
    std::string m_virt_database;
    std::string m_target_address;
    std::string m_target_database;
    std::string m_type;
    bool m_default;
};

class P2_ConfigModule {
 public:
    std::string m_fname;
};

class P2_Config {
    class Rep;
 public:
    P2_Config::P2_Config();
    P2_Config::~P2_Config();
    bool P2_Config::parse_options(int argc, char **argv);
    P2_ConfigTarget *find_target(std::string db);
    void print();
 private:
    bool read_xml_config(const char *fname);
    void parse_xml_element_proxy(void *xml_ptr);
    void parse_xml_element_target(void *xml_ptr,
					     P2_ConfigTarget *t);
    bool parse_xml_text(void *xml_ptr, std::string &val);
    bool parse_xml_text(void *xml_ptr, bool &val);
 public:
    std::string m_apdu_log;
    std::string m_default_target;
    std::string m_listen_address;
    std::string m_log_file;
    std::string m_optimize_flags;
    std::string m_pid_fname;
    std::string m_uid;
    std::string m_xml_fname;

    int m_max_clients;
    int m_client_idletime;
    int m_debug_mode;
    int m_no_limit_files;
    int m_no_threads;
    int m_target_idletime;

    std::list<P2_ConfigTarget *> m_target_list;
    std::list<P2_ConfigModule *> m_modules;
 private:
    Rep *m_rep;
    int m_errors;
};

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
