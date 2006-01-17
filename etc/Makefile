# $Id: Makefile,v 1.4 2006-01-17 10:28:38 mike Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

test: test-config0 test-config1 test-config2 test-config3 test-config-zurg

test-%: %.xml config.xsd 
	xmllint --noout --schema config.xsd $<

