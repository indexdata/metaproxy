# $Id: Makefile,v 1.2 2006-01-16 11:32:40 mike Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

test: test-config0 test-config1 test-config2

test-%: %.xml config.xsd 
	xmllint --noout --schema config.xsd $<

