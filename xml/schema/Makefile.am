# $Id: Makefile.am,v 1.2 2006-10-11 13:21:52 marc Exp $

xmlschemadatadir = $(pkgdatadir)/xml/schema

schemas =  metaproxy.rnc  metaproxy.rng  metaproxy.xsd

xmlschemadata_DATA = $(schemas)

EXTRA_DIST = $(xmlschemadata_DATA)



# pattern compile RelaxNG Compact to RelaxNG XML
.rnc.rng:
	trang $< $@  

# pattern compile RelaxNG Compact to XML Schema
.rnc.xsd:
	trang $< $@  

