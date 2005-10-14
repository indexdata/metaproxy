## $Id: Makefile.am,v 1.18 2005-10-14 11:18:59 adam Exp $

MAINTAINERCLEANFILES = Makefile.in config.in config.hpp

AM_CXXFLAGS = $(YAZPPINC) $(XSLT_CFLAGS)

# Rules for the library..

lib_LTLIBRARIES = libyp2.la
libyp2_la_LDFLAGS = -version-info 0:0:0

libyp2_la_SOURCES = filter_frontend_net.cpp filter_frontend_net.hpp \
	session.cpp session.hpp package.hpp filter.hpp router.hpp \
	thread_pool_observer.cpp thread_pool_observer.hpp

# Rules for programs..

LDADD= libyp2.la $(YAZPPLALIB) $(XSLT_LIBS)

bin_PROGRAMS =
noinst_PROGRAMS = ex_filter_frontend_net

ex_filter_frontend_net_SOURCES = ex_filter_frontend_net.cpp

# Rules for test programs..

check_PROGRAMS = \
	test_package1 \
	test_filter1 test_filter2 \
	test_session1 test_session2 \
	test_thread_pool_observer \
	test_boost_threads test_boost_time \
	test_filter_frontend_net

TESTS=$(check_PROGRAMS)

test_package1_SOURCES=test_package1.cpp
test_filter1_SOURCES=test_filter1.cpp
test_filter2_SOURCES=test_filter2.cpp
test_session1_SOURCES=test_session1.cpp
test_session2_SOURCES=test_session2.cpp
test_boost_threads_SOURCES=test_boost_threads.cpp
test_boost_time_SOURCES=test_boost_time.cpp
test_thread_pool_observer_SOURCES = test_thread_pool_observer.cpp
test_filter_frontend_net_SOURCES = test_filter_frontend_net.cpp

TESTLDADD = $(LDADD) -lboost_unit_test_framework

test_filter1_LDADD = $(TESTLDADD)
test_filter2_LDADD = $(TESTLDADD)
test_session1_LDADD = $(TESTLDADD)
test_session2_LDADD = $(TESTLDADD)
test_boost_threads_LDADD = $(TESTLDADD)
test_boost_time_LDADD = $(TESTLDADD)
test_thread_pool_observer_LDADD = $(TESTLDADD)
test_package1_LDADD = $(TESTLDADD)
test_filter_frontend_net_LDADD = $(TESTLDADD)

# doxygen target
dox:
	(cd $(top_srcdir) ; make dox) 

showdox:
	(cd $(top_srcdir) ; make showdox) 
