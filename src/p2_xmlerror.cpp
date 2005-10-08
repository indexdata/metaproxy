/* $Id: p2_xmlerror.cpp,v 1.2 2005-10-08 23:29:32 adam Exp $
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

#include "config.hpp"
#include <stdio.h>
#include <string.h>
#include <yaz/log.h>

#include "p2_xmlerror.h"

#if HAVE_XSLT
#include <libxml/parser.h>
#include <libxslt/xsltutils.h>
#endif

#if HAVE_XSLT
static void p2_xml_error_handler(void *ctx, const char *fmt, ...)
{
    char buf[1024];
    size_t sz;

    va_list ap;
    va_start(ap, fmt);

#ifdef WIN32
    vsprintf(buf, fmt, ap);
#else
    vsnprintf(buf, sizeof(buf), fmt, ap);
#endif
    sz = strlen(buf);
    if (sz > 0 && buf[sz-1] == '\n')
        buf[sz-1] = '\0';
        
    yaz_log(YLOG_WARN, "%s: %s", (char*) ctx, buf);

    va_end (ap);
}
#endif

void p2_xmlerror_setup()
{
#if HAVE_XSLT
    xmlSetGenericErrorFunc((void *) "XML", p2_xml_error_handler);
    xsltSetGenericErrorFunc((void *) "XSLT", p2_xml_error_handler);
#endif
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */
