
xmlschemadatadir = $(pkgdatadir)/xml/schema

schemas =  metaproxy.rnc  metaproxy.rng  metaproxy.xsd \
   retrievalinfo.rnc  retrievalinfo.rng  retrievalinfo.xsd

xmlschemadata_DATA = $(schemas)

EXTRA_DIST = $(xmlschemadata_DATA)



# pattern compile RelaxNG Compact to RelaxNG XML
.rnc.rng:
	trang $< $@  

# pattern compile RelaxNG Compact to XML Schema
.rnc.xsd:
	trang $< $@  


# needed to get dependencies correct
metaproxy.rng:metaproxy.rnc retrievalinfo.rnc

metaproxy.xsd:metaproxy.rnc retrievalinfo.rnc
