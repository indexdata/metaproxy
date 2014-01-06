## This file is part of Metaproxy
## Copyright (C) Index Data
## See the file LICENSE for details.

MAINTAINERCLEANFILES = Makefile.in config.in config.hpp

DISTCLEANFILES = metaproxy-config

AM_CXXFLAGS = $(BOOST_CPPFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/include $(USEMARCONINC)

bin_SCRIPTS = metaproxy-config

filter_src = \
	filter_auth_simple.cpp filter_auth_simple.hpp \
	filter_backend_test.cpp filter_backend_test.hpp \
	filter_bounce.cpp filter_bounce.hpp \
	filter_cgi.cpp filter_cgi.hpp \
	filter_cql_rpn.cpp filter_cql_rpn.hpp \
	filter_frontend_net.cpp filter_frontend_net.hpp \
	filter_http_client.cpp filter_http_client.hpp \
	filter_http_file.cpp filter_http_file.hpp \
	filter_http_rewrite1.cpp filter_http_rewrite1.hpp \
	filter_http_rewrite.cpp filter_http_rewrite.hpp \
	filter_limit.cpp filter_limit.hpp \
	filter_load_balance.cpp filter_load_balance.hpp \
	filter_log.cpp filter_log.hpp \
	filter_multi.cpp filter_multi.hpp \
	filter_query_rewrite.cpp filter_query_rewrite.hpp \
	filter_present_chunk.cpp filter_present_chunk.hpp \
	filter_record_transform.cpp filter_record_transform.hpp \
	filter_session_shared.cpp filter_session_shared.hpp \
	filter_sd_remove.cpp filter_sd_remove.hpp \
	filter_sort.cpp filter_sort.hpp \
        filter_sru_to_z3950.cpp  filter_sru_to_z3950.hpp \
	filter_template.cpp filter_template.hpp \
	filter_virt_db.cpp filter_virt_db.hpp \
	filter_z3950_client.cpp filter_z3950_client.hpp \
	filter_zeerex_explain.cpp  filter_zeerex_explain.hpp \
	filter_zoom.cpp filter_zoom.hpp

lib_LTLIBRARIES = libmetaproxy.la
libmetaproxy_la_LDFLAGS = -version-info 6:0:0 -export-dynamic

libmetaproxy_la_SOURCES = \
	$(filter_src) \
	factory_filter.cpp factory_filter.hpp \
	factory_static.cpp factory_static.hpp \
	filter.cpp \
	gduutil.cpp gduutil.hpp \
	origin.cpp \
	package.cpp \
	pipe.cpp pipe.hpp \
	plainfile.cpp \
	session.cpp \
	sru_util.cpp sru_util.hpp \
	thread_pool_observer.cpp thread_pool_observer.hpp \
	torus.cpp torus.hpp \
	url_recipe.cpp \
	util.cpp \
	html_parser.hpp html_parser.cpp \
	router_chain.cpp \
        router_flexml.hpp router_flexml.cpp \
	router_xml.cpp \
	xmlutil.cpp

libmetaproxy_la_LIBADD = $(YAZPPLALIB) \
	$(BOOST_LIB) $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) \
	$(BOOST_REGEX_LIB)

metaproxy_LDADD = libmetaproxy.la $(YAZPPLALIB) \
	$(BOOST_LIB) $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) \
	$(BOOST_REGEX_LIB) $(USEMARCONLALIB)

tstdl_LDADD = $(metaproxy_LDADD)

LDADD = $(metaproxy_LDADD) $(BOOST_TEST_LIB)
AM_LDFLAGS = -export-dynamic

bin_PROGRAMS = metaproxy
noinst_PROGRAMS = tstdl

tstdl_SOURCES = tstdl.cpp
metaproxy_SOURCES = metaproxy_prog.cpp

# Rules for dl programs
pkglib_LTLIBRARIES = metaproxy_filter_dl.la

metaproxy_filter_dl_la_SOURCES = filter_dl.cpp
metaproxy_filter_dl_la_LDFLAGS = -rpath $(pkglibdir) -module -avoid-version
metaproxy_filter_dl_la_LIBADD = libmetaproxy.la

# Rules for test programs..

check_PROGRAMS = \
	test_package1 \
	test_pipe \
	test_filter1 test_filter2 \
	test_session1 test_session2 \
	test_thread_pool_observer \
	test_boost_threads \
	test_filter_auth_simple \
	test_filter_factory \
	test_filter_frontend_net \
	test_filter_log \
	test_filter_multi \
	test_filter_query_rewrite \
	test_filter_z3950_client \
	test_filter_backend_test \
	test_filter_bounce \
	test_filter_record_transform \
	test_filter_sru_to_z3950 \
	test_filter_virt_db \
	test_filter_rewrite \
	test_ses_map \
	test_router_flexml \
	test_html_parser \
	test_xmlutil

TESTS=$(check_PROGRAMS)

test_package1_SOURCES=test_package1.cpp
test_pipe_SOURCES=test_pipe.cpp
test_filter1_SOURCES=test_filter1.cpp
test_filter2_SOURCES=test_filter2.cpp
test_session1_SOURCES=test_session1.cpp
test_session2_SOURCES=test_session2.cpp
test_boost_threads_SOURCES=test_boost_threads.cpp
test_thread_pool_observer_SOURCES = test_thread_pool_observer.cpp
test_filter_auth_simple_SOURCES = test_filter_auth_simple.cpp
test_filter_factory_SOURCES = test_filter_factory.cpp
test_filter_frontend_net_SOURCES = test_filter_frontend_net.cpp
test_filter_log_SOURCES = test_filter_log.cpp
test_filter_multi_SOURCES = test_filter_multi.cpp
test_filter_query_rewrite_SOURCES = test_filter_query_rewrite.cpp
test_filter_z3950_client_SOURCES = test_filter_z3950_client.cpp
test_filter_backend_test_SOURCES = test_filter_backend_test.cpp
test_filter_bounce_SOURCES = test_filter_bounce.cpp
test_filter_record_transform_SOURCES = test_filter_record_transform.cpp
test_filter_sru_to_z3950_SOURCES = test_filter_sru_to_z3950.cpp
test_filter_virt_db_SOURCES = test_filter_virt_db.cpp
test_ses_map_SOURCES = test_ses_map.cpp
test_router_flexml_SOURCES = test_router_flexml.cpp
test_xmlutil_SOURCES = test_xmlutil.cpp

test_html_parser_SOURCES = test_html_parser.cpp
test_filter_rewrite_SOURCES = test_filter_rewrite.cpp

# doxygen target
dox:
	(cd $(top_srcdir) ; make dox)

showdox:
	(cd $(top_srcdir) ; make showdox)
