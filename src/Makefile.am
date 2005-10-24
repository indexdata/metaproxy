## $Id: Makefile.am,v 1.22 2005-10-24 13:31:36 marc Exp $

MAINTAINERCLEANFILES = Makefile.in config.in config.hpp

AM_CXXFLAGS = $(YAZPPINC) $(XSLT_CFLAGS)

# Rules for the library..

lib_LTLIBRARIES = libyp2.la
libyp2_la_LDFLAGS = -version-info 0:0:0

libyp2_la_SOURCES = \
	session.cpp session.hpp package.hpp filter.hpp router.hpp \
	thread_pool_observer.cpp thread_pool_observer.hpp \
	filter_frontend_net.cpp filter_frontend_net.hpp \
	filter_log.cpp filter_log.hpp \
	filter_z3950_client.cpp filter_z3950_client.hpp

# Rules for programs..

LDADD= libyp2.la $(YAZPPLALIB) $(XSLT_LIBS)

bin_PROGRAMS =
noinst_PROGRAMS = ex_filter_frontend_net ex_libxml2_conf

ex_filter_frontend_net_SOURCES = ex_filter_frontend_net.cpp
ex_libxml2_conf_SOURCES = ex_libxml2_conf.cpp

# Rules for test programs..

check_PROGRAMS = \
	test_package1 \
	test_filter1 test_filter2 \
	test_session1 test_session2 \
	test_thread_pool_observer \
	test_boost_threads test_boost_time \
	test_filter_frontend_net \
	test_filter_log \
	test_filter_z3950_client

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
test_filter_log_SOURCES = test_filter_log.cpp
test_filter_z3950_client_SOURCES = test_filter_z3950_client.cpp

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
test_filter_log_LDADD = $(TESTLDADD)
test_filter_z3950_client_LDADD = $(TESTLDADD)

# doxygen target
dox:
	(cd $(top_srcdir) ; make dox) 

showdox:
	(cd $(top_srcdir) ; make showdox) 
