# $Id: Makefile.am,v 1.5 2006-10-10 15:41:51 marc Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

etcdatadir = $(pkgdatadir)/etc

xmlconfig = config-bytarget.xml \
    config-local.xml \
    config-record-transform.xml \
    config-shared1.xml \
    config-simple-multi.xml \
    config-sru-to-z3950.xml \
    config-zurg.xml \
    config0.xml \
    config1.xml \
    config2.xml \
    config3.xml \
    config4.xml \
    config5.xml

config = example.simple-auth example.target-auth pqf2pqf.xsl


rng = ../xml/schema/metaproxy.rng
xsd = ../xml/schema/metaproxy.xsd

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


