## This file is part of Metaproxy
## Copyright (C) 2005-2010 Index Data

SUBDIRS = xml include src etc doc

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = Doxyfile.in NEWS README LICENSE \
             m4/yazpp.m4 m4/yaz.m4 m4/ax_boost.m4 \
             buildconf.sh

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure 

dist-hook:
	if test -x /usr/bin/git -a -d .git; then git log >ChangeLog ; cp ChangeLog $(distdir); fi
	mkdir $(distdir)/win
	-cp $(srcdir)/win/* $(distdir)/win

.PHONY:dox
dox:
	doxygen

.PHONY:debian
debian:
	dpkg-buildpackage -rfakeroot
