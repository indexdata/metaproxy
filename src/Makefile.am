## $Id: Makefile.am,v 1.2 2005-10-06 09:53:11 marc Exp $

AM_CXXFLAGS = $(YAZPPINC) $(XSLT_CFLAGS) $(USEMARCONINC)

bin_PROGRAMS =
check_PROGRAMS =  
noinst_PROGRAMS =  p2 design

TESTS=$(check_PROGRAMS)

design_SOURCES=design.h  design_main.cpp


p2_SOURCES=p2_frontend.cpp p2_msg.cpp p2.cpp p2_frontend.h \
 p2_config.cpp p2_config.h \
 p2_backend.h p2_backend_dummy.cpp \
 p2_modules.cpp p2_modules.h \
 p2_xmlerror.cpp p2_xmlerror.h msg-thread.h msg-thread.cpp

LDADD= $(YAZPPLALIB) $(XSLT_LIBS) $(USEMARCONLALIB)


# Modules
