## This file is part of Metaproxy
## Copyright (C) 2005-2006, Index Data ApS
## $Id: Makefile.am,v 1.17 2006-09-29 09:43:56 marc Exp $

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

.PHONY:firefox
firefox:
	firefox  -new-window file://`pwd`/dox/html/index.html &

.PHONY:konqueror
konqueror:
	konqueror file://`pwd`/dox/html/index.html &

.PHONY:galeon
galeon:
	galeon file://`pwd`/dox/html/index.html &

.PHONY:epiphany
epiphany:
	epiphany --new-window file://`pwd`/dox/html/index.html &

