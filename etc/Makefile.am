# $Id: Makefile.am,v 1.6 2006-10-11 13:21:51 marc Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

etcdatadir = $(pkgdatadir)/etc

xmlconfig = $(srcdir)/config-bytarget.xml \
    $(srcdir)/config-local.xml \
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
    $(srcdir)/config5.xml

config = example.simple-auth example.target-auth pqf2pqf.xsl


rng = $(srcdir)/../xml/schema/metaproxy.rng
xsd = $(srcdir)/../xml/schema/metaproxy.xsd

etcdata_DATA = $(xmlconfig) $(config)

EXTRA_DIST = $(etcdata_DATA)

.PHONY: check_rng
check_rng: 
	xmllint --noout --relaxng $(rng) $(xmlconfig)

.PHONY: check_xsd
check_xsd: 
	xmllint --noout --schema $(xsd) $(xmlconfig)


check-local:
	$(MAKE) check_xsd
	$(MAKE) check_rng


