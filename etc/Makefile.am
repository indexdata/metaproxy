# $Id: Makefile.am,v 1.12 2007-02-17 13:03:29 adam Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

etcdatadir = $(pkgdatadir)/etc

xmlconfig = $(srcdir)/config-bytarget.xml \
    $(srcdir)/config-local.xml \
    $(srcdir)/config-load-balance.xml \
    $(srcdir)/config-record-transform.xml \
    $(srcdir)/config-shared1.xml \
    $(srcdir)/config-simple-multi.xml \
    $(srcdir)/config-sru-to-z3950.xml \
    $(srcdir)/config-zurg.xml \
    $(srcdir)/config0.xml \
    $(srcdir)/config1.xml \
    $(srcdir)/config2.xml \
    $(srcdir)/config3.xml \
    $(srcdir)/config4.xml \
    $(srcdir)/config5.xml \
    $(srcdir)/retrieval-info.xml

config = example.simple-auth example.target-auth pqf2pqf.xsl explain.xml


rng = $(srcdir)/../xml/schema/metaproxy.rng
xsd = $(srcdir)/../xml/schema/metaproxy.xsd

etcdata_DATA = $(xmlconfig) $(config)

EXTRA_DIST = $(etcdata_DATA)

.PHONY: check_rng
check_rng: 
	xmllint --noout --xinclude --relaxng $(rng) $(xmlconfig)

## Only run XSD checks for a "new" Libxml2
.PHONY: check_xsd
check_xsd: 
	if test `xmllint --version 2>&1 |head -1|awk '{print $$5}'` -gt 20622; then xmllint --noout --xinclude --schema $(xsd) $(xmlconfig); fi

check-local: check_rng check_xsd


