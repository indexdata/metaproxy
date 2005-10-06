## $Id: Makefile.am,v 1.4 2005-10-06 19:33:58 adam Exp $

AM_CXXFLAGS = $(YAZPPINC) $(XSLT_CFLAGS) $(USEMARCONINC)

bin_PROGRAMS =
check_PROGRAMS = test_filter1 test_filter2 test_thread_pool_observer
noinst_PROGRAMS =  p2 design

TESTS=$(check_PROGRAMS)

design_SOURCES=design.h  design_main.cpp

test_filter1_SOURCES=test_filter1.cpp
test_filter2_SOURCES=test_filter2.cpp

p2_SOURCES=p2_frontend.cpp p2_msg.cpp p2.cpp p2_frontend.h \
 p2_config.cpp p2_config.h \
 p2_backend.h p2_backend_dummy.cpp \
 p2_modules.cpp p2_modules.h \
 p2_xmlerror.cpp p2_xmlerror.h \
 thread_pool_observer.cpp thread_pool_observer.h

test_thread_pool_observer_SOURCES = test_thread_pool_observer.cpp \
 thread_pool_observer.cpp thread_pool_observer.h

LDADD= $(YAZPPLALIB) $(XSLT_LIBS) $(USEMARCONLALIB)


# Modules
