## This file is part of Metaproxy
## Copyright (C) Index Data
## See the file LICENSE for details.

SUBDIRS = xml include src etc doc

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = Doxyfile.in IDMETA README.md LICENSE NEWS m4/id-config.sh \
             m4/yazpp.m4 m4/yaz.m4 m4/boost.m4 \
             buildconf.sh metaproxy.spec metaproxy-config.in

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = metaproxy.pc

dist-hook:
	if test -x /usr/bin/git -a -d .git; then git log >ChangeLog ; cp ChangeLog $(distdir); fi
	mkdir $(distdir)/win
	-cp $(srcdir)/win/* $(distdir)/win
	mkdir $(distdir)/rpm
	-cp $(srcdir)/rpm/* $(distdir)/rpm

.PHONY:dox
dox:
	doxygen

.PHONY:debian
debian:
	dpkg-buildpackage -rfakeroot
