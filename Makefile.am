## This file is part of Metaproxy
## Copyright (C) 2005-2007, Index Data ApS
## $Id: Makefile.am,v 1.24 2008-02-14 13:46:46 marc Exp $

SUBDIRS = xml src etc doc

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = Doxyfile.in NEWS README LICENSE \
             m4/yazpp.m4 m4/yaz.m4 m4/ax_boost.m4 \
             buildconf.sh

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure 

dist-hook:
	if test -x /usr/bin/cvs2cl -a -d CVS; then cvs2cl ; fi
	mkdir $(distdir)/win
	-cp $(srcdir)/win/* $(distdir)/win
	mkdir $(distdir)/debian
	cp $(srcdir)/debian/control $(distdir)/debian
	cp $(srcdir)/debian/copyright $(distdir)/debian
	cp $(srcdir)/debian/changelog $(distdir)/debian
	cp $(srcdir)/debian/rules $(distdir)/debian
	cp $(srcdir)/debian/*.install $(distdir)/debian
	cp $(srcdir)/debian/*.manpages $(distdir)/debian

#cp $(srcdir)/debian/*.postinst $(distdir)/debian


.PHONY:dox
dox:
	doxygen

.PHONY:debian
debian:
	dpkg-buildpackage -rfakeroot
