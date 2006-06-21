## This file is part of Metaproxy
## Copyright (C) 2005-2006, Index Data ApS
## $Id: Makefile.am,v 1.15 2006-06-21 14:37:52 adam Exp $

SUBDIRS = src doc etc

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = Doxyfile.in NEWS README LICENSE m4/yazpp.m4 m4/yaz.m4 buildconf.sh

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure 

dist-hook:
	if test -x /usr/bin/cvs2cl -a -d CVS; then cvs2cl ; fi
	mkdir $(distdir)/win
	-cp $(srcdir)/win/* $(distdir)/win

# doxygen target
.PHONY:dox
dox:
	doxygen

.PHONY:showdox
showdox:
	firefox file://`pwd`/dox/html/index.html &
