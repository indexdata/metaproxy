## This file is part of Metaproxy
## Copyright (C) 2005-2006, Index Data ApS
## $Id: Makefile.am,v 1.12 2006-04-27 08:51:31 adam Exp $

SUBDIRS = src doc

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = Doxyfile.in NEWS README m4/yazpp.m4 m4/yaz.m4 buildconf.sh

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure 

dist-hook:
	mkdir $(distdir)/win
	-cp $(srcdir)/win/* $(distdir)/win

# doxygen target
.PHONY:dox
dox:
	doxygen

.PHONY:showdox
showdox:
	firefox file://`pwd`/dox/html/index.html &
