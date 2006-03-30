# $Id: Makefile,v 1.5 2006-03-30 11:15:48 mike Exp $

# This doesn't actually build anything, it just tests whether the
# configuration files here correspond with the schema.

test: test-config0 test-config1 test-config2 test-config3 test-config4 \
	test-config-zurg
# 5 imagine

test-%: %.xml config.xsd 
	xmllint --noout --schema config.xsd $<

