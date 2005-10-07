AUTOMAKE_OPTIONS = foreign

SUBDIRS = src 

EXTRA_DIST= m4/yazpp.m4 m4/yaz.m4  buildconf.sh

MAINTAINERCLEANFILES    = Makefile.in aclocal.m4 configure 

# doxygen target
.PHONY:dox
dox:
	doxygen
