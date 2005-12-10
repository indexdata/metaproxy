## $Id: Makefile.am,v 1.38 2005-12-10 09:59:10 adam Exp $

MAINTAINERCLEANFILES = Makefile.in config.in config.hpp

AM_CXXFLAGS = $(YAZPPINC) $(XSLT_CFLAGS)

EXTRA_DIST=Jamfile.in

# Rules for the library..

lib_LTLIBRARIES = libyp2.la
libyp2_la_LDFLAGS = -version-info 0:0:0 -export-dynamic

libyp2_la_SOURCES = \
	session.cpp session.hpp \
	package.cpp package.hpp \
	router.hpp router_chain.hpp router_chain.cpp \
        router_flexml.hpp router_flexml.cpp \
	thread_pool_observer.cpp thread_pool_observer.hpp \
	filter.hpp filter.cpp filter_factory.cpp filter_factory.hpp \
	filter_frontend_net.cpp filter_frontend_net.hpp \
	filter_log.cpp filter_log.hpp \
	filter_virt_db.cpp filter_virt_db.hpp \
	filter_z3950_client.cpp filter_z3950_client.hpp \
	filter_backend_test.cpp filter_backend_test.hpp \
	filter_session_shared.cpp filter_session_shared.hpp \
	filter_template.cpp filter_template.hpp \
	pipe.cpp pipe.hpp \
	util.cpp util.hpp

# Rules for programs..

LDADD= libyp2.la $(YAZPPLALIB) $(XSLT_LIBS)

bin_PROGRAMS =
noinst_PROGRAMS = ex_filter_frontend_net ex_router_flexml tstdl

ex_filter_frontend_net_SOURCES = ex_filter_frontend_net.cpp
ex_router_flexml_SOURCES =  ex_router_flexml.cpp
tstdl_SOURCES = tstdl.cpp

# Rules for dl programs
pkglib_LTLIBRARIES = yp2_filter_dl.la

yp2_filter_dl_la_SOURCES = filter_dl.cpp
yp2_filter_dl_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
yp2_filter_dl_la_LIBADD = libyp2.la

# Rules for test programs..

check_PROGRAMS = \
	test_package1 \
	test_pipe \
	test_filter1 test_filter2 \
	test_session1 test_session2 \
	test_thread_pool_observer \
	test_boost_threads test_boost_time \
	test_filter_factory \
	test_filter_frontend_net \
	test_filter_log \
	test_filter_z3950_client \
	test_filter_backend_test \
	test_filter_virt_db \
	test_ses_map \
	test_router_flexml

TESTS=$(check_PROGRAMS)

test_package1_SOURCES=test_package1.cpp
test_pipe_SOURCES=test_pipe.cpp
test_filter1_SOURCES=test_filter1.cpp
test_filter2_SOURCES=test_filter2.cpp
test_session1_SOURCES=test_session1.cpp
test_session2_SOURCES=test_session2.cpp
test_boost_threads_SOURCES=test_boost_threads.cpp
test_boost_time_SOURCES=test_boost_time.cpp
test_thread_pool_observer_SOURCES = test_thread_pool_observer.cpp
test_filter_factory_SOURCES = test_filter_factory.cpp
test_filter_factory_LDFLAGS = -export-dynamic
test_filter_frontend_net_SOURCES = test_filter_frontend_net.cpp
test_filter_log_SOURCES = test_filter_log.cpp
test_filter_z3950_client_SOURCES = test_filter_z3950_client.cpp
test_filter_backend_test_SOURCES = test_filter_backend_test.cpp
test_filter_virt_db_SOURCES = test_filter_virt_db.cpp
test_ses_map_SOURCES = test_ses_map.cpp
test_router_flexml_SOURCES = test_router_flexml.cpp

TESTLDADD = $(LDADD) -lboost_unit_test_framework

test_package1_LDADD = $(TESTLDADD)
test_pipe_LDADD = $(TESTLDADD)
test_filter1_LDADD = $(TESTLDADD)
test_filter2_LDADD = $(TESTLDADD)
test_session1_LDADD = $(TESTLDADD)
test_session2_LDADD = $(TESTLDADD)
test_boost_threads_LDADD = $(TESTLDADD)
test_boost_time_LDADD = $(TESTLDADD)
test_thread_pool_observer_LDADD = $(TESTLDADD)
test_filter_factory_LDADD = $(TESTLDADD)
test_filter_frontend_net_LDADD = $(TESTLDADD)
test_filter_log_LDADD = $(TESTLDADD)
test_filter_z3950_client_LDADD = $(TESTLDADD)
test_filter_backend_test_LDADD = $(TESTLDADD)
test_filter_virt_db_LDADD = $(TESTLDADD)
test_ses_map_LDADD = $(TESTLDADD)
test_router_flexml_LDADD = $(TESTLDADD)

# doxygen target
dox:
	(cd $(top_srcdir) ; make dox) 

showdox:
	(cd $(top_srcdir) ; make showdox) 
