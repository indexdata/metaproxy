# $Id: Makefile,v 1.3 2006-01-16 12:02:00 mike Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

test: test-config0 test-config1 test-config2 test-config3

test-%: %.xml config.xsd 
	xmllint --noout --schema config.xsd $<

