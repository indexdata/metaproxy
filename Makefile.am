## This file is part of Metaproxy
## Copyright (C) 2005-2008 Index Data

SUBDIRS = xml src etc doc

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
	mkdir $(distdir)/debian
	cp $(srcdir)/debian/control $(distdir)/debian
	cp $(srcdir)/debian/copyright $(distdir)/debian
	cp $(srcdir)/debian/changelog $(distdir)/debian
	cp $(srcdir)/debian/rules $(distdir)/debian
	cp $(srcdir)/debian/*.install $(distdir)/debian
	cp $(srcdir)/debian/metaproxy.default $(distdir)/debian
	cp $(srcdir)/debian/metaproxy.init $(distdir)/debian
	cp $(srcdir)/debian/metaproxy.logrotate $(distdir)/debian

#cp $(srcdir)/debian/*.manpages $(distdir)/debian
#cp $(srcdir)/debian/*.postinst $(distdir)/debian


.PHONY:dox
dox:
	doxygen

.PHONY:debian
debian:
	dpkg-buildpackage -rfakeroot
