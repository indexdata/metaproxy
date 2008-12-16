
TRANG=trang
# TRANG=java -jar /home/adam/proj/trang-20081028/trang.jar

xmlschemadatadir = $(pkgdatadir)/xml/schema

schemas =  metaproxy.rnc  metaproxy.rng  metaproxy.xsd \
   retrievalinfo.rnc  retrievalinfo.rng  retrievalinfo.xsd

xmlschemadata_DATA = $(schemas)

EXTRA_DIST = $(xmlschemadata_DATA)

# pattern compile RelaxNG Compact to RelaxNG XML
.rnc.rng:
	$(TRANG) $< $@  

# pattern compile RelaxNG Compact to XML Schema
.rnc.xsd:
	$(TRANG) $< $@  


# needed to get dependencies correct
metaproxy.rng:metaproxy.rnc retrievalinfo.rnc

metaproxy.xsd:metaproxy.rnc retrievalinfo.rnc
