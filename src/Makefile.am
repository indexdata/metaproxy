## $Id: Makefile.am,v 1.9 2005-10-07 13:56:02 adam Exp $

AM_CXXFLAGS = $(YAZPPINC) $(XSLT_CFLAGS)

YP2_INCHPP = session.hpp package.hpp filter.hpp router.hpp

bin_PROGRAMS =
check_PROGRAMS = test_filter1 test_filter2 test_session1 \
                test_thread_pool_observer test_boost_threads
noinst_PROGRAMS =  p2 

TESTS=$(check_PROGRAMS)

test_filter1_SOURCES=test_filter1.cpp $(YP2_INCHPP)
test_filter2_SOURCES=test_filter2.cpp $(YP2_INCHPP)
test_session1_SOURCES=test_session1.cpp $(YP2_INCHPP)
test_boost_threads_SOURCES=test_boost_threads.cpp
test_thread_pool_observer_SOURCES = test_thread_pool_observer.cpp \
          thread_pool_observer.cpp thread_pool_observer.h

p2_SOURCES=p2_frontend.cpp p2_msg.cpp p2.cpp p2_frontend.h \
 p2_config.cpp p2_config.h \
 p2_backend.h p2_backend_dummy.cpp \
 p2_modules.cpp p2_modules.h \
 p2_xmlerror.cpp p2_xmlerror.h \
 thread_pool_observer.cpp thread_pool_observer.h


LDADD= $(YAZPPLALIB) $(XSLT_LIBS)

test_filter1_LDADD = $(LDADD) -lboost_unit_test_framework
test_boost_threads_LDADD = $(LDADD) -lboost_unit_test_framework

