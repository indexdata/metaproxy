## $Id: Makefile.am,v 1.1 2005-10-06 09:37:25 marc Exp $

AM_CXXFLAGS = $(YAZPPINC) -I$(srcdir)/../include $(XSLT_CFLAGS) $(USEMARCONINC)

lib_LTLIBRARIES = libyazproxy.la
libyazproxy_la_LDFLAGS=-version-info 1:0:0

libyazproxy_la_SOURCES= yaz-proxy.cpp yaz-proxy-config.cpp yaz-bw.cpp \
 proxyp.h yaz-usemarcon.cpp charset-converter.cpp msg-thread.cpp msg-thread.h \
 modules.cpp

bin_PROGRAMS = yazproxy
check_PROGRAMS = cdetails
noinst_PROGRAMS = tstthreads t-server p2

TESTS=$(check_PROGRAMS)

yazproxy_SOURCES=yaz-proxy-main.cpp
cdetails_SOURCES=cdetails.cpp
tstthreads_SOURCES=tstthreads.cpp
t_server_SOURCES=t-server.cpp
p2_SOURCES=p2_frontend.cpp p2_msg.cpp p2.cpp p2_frontend.h \
 p2_config.cpp p2_config.h \
 p2_backend.h p2_backend_dummy.cpp \
 p2_modules.cpp p2_modules.h \
 p2_xmlerror.cpp p2_xmlerror.h

LDADD=libyazproxy.la $(YAZPPLALIB) $(XSLT_LIBS) $(USEMARCONLALIB)
libyazproxy_la_LIBADD = $(XSLT_LIBS)

# Modules
mod_proxy_sample_la_SOURCES = mod_sample.cpp
mod_proxy_sample_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version

pkglib_LTLIBRARIES = mod_proxy_sample.la
