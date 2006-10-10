# $Id: Makefile.am,v 1.1 2006-10-10 15:10:18 marc Exp $

xmlschemadatadir = $(pkgdatadir)/xml/schema

schemas =  metaproxy.rnc  metaproxy.rng  metaproxy.xsd

xmlschemadata_DATA = $(schemas)

EXTRA_DIST = $(xmlschemadata_DATA)



# pattern compile RelaxNG Compact to RelaxNG XML
%.rng : %.rnc
	trang $< $@  

# pattern compile RelaxNG Compact to XML Schema
%.xsd : %.rnc 
	trang $< $@  

