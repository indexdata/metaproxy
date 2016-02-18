
TRANG=/usr/bin/trang

xmlschemadatadir = $(pkgdatadir)/xml/schema

rnc =  \
filter_auth_simple.rnc \
filter_backend_test.rnc \
filter_bounce.rnc \
filter_cgi.rnc \
filter_cql_rpn.rnc \
filter_frontend_net.rnc \
filter_http_client.rnc \
filter_http_file.rnc \
filter_http_rewrite.rnc \
filter_http_rewrite1.rnc \
filter_limit.rnc \
filter_load_balance.rnc \
filter_log.rnc \
filter_multi.rnc \
filter_present_chunk.rnc \
filter_query_rewrite.rnc \
filter_record_transform.rnc \
filter_sd_remove.rnc \
filter_session_shared.rnc \
filter_sort.rnc \
filter_sru_z3950.rnc \
filter_virt_db.rnc \
filter_z3950_client.rnc \
filter_zoom.rnc \
metaproxy.rnc \
retrievalinfo.rnc

noinst_DATA = metaproxy.rng metaproxy.xsd

xmlschemadata_DATA = $(rnc)

EXTRA_DIST = $(rnc)

clean-local:
	rm -f *.xsd *.rng

metaproxy.rng:$(rnc)
	if test -x $(TRANG); then \
		$(TRANG) $(srcdir)/metaproxy.rnc metaproxy.rng; \
	fi

metaproxy.xsd:$(rnc)
	if test -x $(TRANG); then \
		$(TRANG) $(srcdir)/metaproxy.rnc metaproxy.xsd; \
	fi
