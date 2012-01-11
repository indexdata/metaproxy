/* This file is part of Metaproxy.
   Copyright (C) 2005-2012 Index Data

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

#include "config.hpp"

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

#include <metaproxy/util.hpp>

#define PLAINFILE_MAX_LINE 256

namespace mp = metaproxy_1;

namespace metaproxy_1 {
    class PlainFile::Rep {
        friend class PlainFile;
        Rep();
        void close();
        int lineno;
        std::ifstream *fh;
    };
}

mp::PlainFile::Rep::Rep() : lineno(1)
{
    fh = 0;
}

mp::PlainFile::PlainFile() : m_p(new Rep)
{
}

void mp::PlainFile::Rep::close()
{
    delete fh;
    fh = 0;
    lineno = 0;
}

mp::PlainFile::~PlainFile()
{
    m_p->close();
}

bool mp::PlainFile::open(const std::string &fname)
{
    m_p->close();

    std::ifstream *new_file = new std::ifstream(fname.c_str());
    if (! *new_file)
    {
        delete new_file;
        return false;
    }
    m_p->fh = new_file;
    return true;
}

bool mp::PlainFile::getline(std::vector<std::string> &args)
{
    args.clear();

    if (!m_p->fh)
        return false;  // no file at all.

    char line_cstr[PLAINFILE_MAX_LINE];
    while (true)
    {
        if (m_p->fh->eof())
        {
            m_p->close();   // might as well close it now
            return false;
        }
        
        m_p->lineno++;
        m_p->fh->getline(line_cstr, PLAINFILE_MAX_LINE-1);
        char first = line_cstr[0];
        if (first && !strchr("# \t", first))
            break;
        // comment or blank line.. read next.
    }
    const char *cp = line_cstr;
    while (true)
    {
        // skip whitespace
        while (*cp && strchr(" \t", *cp))
            cp++;
        if (*cp == '\0')
            break;
        const char *cp0 = cp;
        while (*cp && !strchr(" \t", *cp))
            cp++;
        std::string arg(cp0, cp - cp0);
        args.push_back(arg);
    }
    return true;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

