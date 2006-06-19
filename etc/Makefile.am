# $Id: Makefile.am,v 1.3 2006-06-19 13:29:30 adam Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

etcdatadir = $(pkgdatadir)/etc

config = config0.xml config1.xml config2.xml \
	config3.xml config4.xml config5.xml \
	config-simple-multi.xml config-local.xml \
	config-shared1.xml

misc = example.simple-auth example.target-auth pqf2pqf.xsl

etcdata_DATA = $(config) $(misc)

EXTRA_DIST = $(etcdata_DATA)

test: test-config0 test-config1 test-config2 \
      test-config3 test-config4 test-config5 \
      test-config-simple-multi test-config-local \
	test-config-shared1

test-%: %.xml config.xsd 
	xmllint --noout --schema config.xsd $<

