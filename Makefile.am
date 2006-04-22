SUBDIRS = src doc

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = Doxyfile.in NEWS README m4/yazpp.m4 m4/yaz.m4 buildconf.sh

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure 

# doxygen target
.PHONY:dox
dox:
	doxygen

.PHONY:showdox
showdox:
	firefox file://`pwd`/dox/html/index.html &
