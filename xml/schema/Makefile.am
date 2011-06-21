
TRANG=trang
# TRANG=java -jar /home/adam/proj/trang-20081028/trang.jar

xmlschemadatadir = $(pkgdatadir)/xml/schema

xsd = \
filter_auth_simple.xsd \
filter_backend_test.xsd \
filter_bounce.xsd \
filter_cgi.xsd \
filter_cql_rpn.xsd \
filter_frontend_net.xsd \
filter_http_file.xsd \
filter_limit.xsd \
filter_load_balance.xsd \
filter_log.xsd \
filter_multi.xsd \
filter_query_rewrite.xsd \
filter_record_transform.xsd \
filter_session_shared.xsd \
filter_sru_z3950.xsd \
filter_virt_db.xsd \
filter_z3950_client.xsd \
filter_zoom.xsd \
local.xsd \
metaproxy.xsd \
retrievalinfo.xsd

rng = \
filter_auth_simple.rng \
filter_backend_test.rng \
filter_bounce.rng \
filter_cgi.rng \
filter_cql_rpn.rng \
filter_frontend_net.rng \
filter_http_file.rng \
filter_limit.rng \
filter_load_balance.rng \
filter_log.rng \
filter_multi.rng \
filter_query_rewrite.rng \
filter_record_transform.rng \
filter_session_shared.rng \
filter_sru_z3950.rng \
filter_virt_db.rng \
filter_z3950_client.rng \
filter_zoom.rng \
metaproxy.rng \
retrievalinfo.rng

rnc =  \
filter_auth_simple.rnc \
filter_backend_test.rnc \
filter_bounce.rnc \
filter_cgi.rnc \
filter_cql_rpn.rnc \
filter_frontend_net.rnc \
filter_http_file.rnc \
filter_limit.rnc \
filter_load_balance.rnc \
filter_log.rnc \
filter_multi.rnc \
filter_query_rewrite.rnc \
filter_record_transform.rnc \
filter_session_shared.rnc \
filter_sru_z3950.rnc \
filter_virt_db.rnc \
filter_z3950_client.rnc \
filter_zoom.rnc \
metaproxy.rnc \
retrievalinfo.rnc

xmlschemadata_DATA = $(xsd) $(rng) $(rnc)

EXTRA_DIST = $(xmlschemadata_DATA)

$(rng):$(rnc)
	$(TRANG) metaproxy.rnc metaproxy.rng

$(xsd):$(rnc)
	$(TRANG) metaproxy.rnc metaproxy.xsd
