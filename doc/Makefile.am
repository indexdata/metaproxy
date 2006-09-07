## $Id: Makefile.am,v 1.26 2006-09-07 09:00:09 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUBDIRS = common

SUFFIXES=.3mp .1

XMLFILES = book.xml manref.xml progref.xml copyright.xml metaproxy.xml

MAINXML = $(srcdir)/book.xml

XMLMAN = auth_simple.xml backend_test.xml frontend_net.xml \
	http_file.xml log.xml multi.xml query_rewrite.xml \
	session_shared.xml template.xml virt_db.xml z3950_client.xml
MANFILES = auth_simple.3mp backend_test.3mp frontend_net.3mp \
	http_file.3mp log.3mp multi.3mp query_rewrite.3mp \
	session_shared.3mp template.3mp virt_db.3mp z3950_client.3mp \
	metaproxy.1

HTMLFILES = index.html

PNGFILES = multi.png

doc_DATA = $(HTMLFILES) $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(XMLMAN) $(doc_DATA) $(man_MANS) multi.svg

$(HTMLFILES): $(XMLFILES) multi.png
	rm -f *.html
	$(HTML_COMPILE) $(MAINXML)

.xml.3mp:
	$(MAN_COMPILE) $<

.xml.1:
	$(MAN_COMPILE) $<

index.tkl: $(XMLFILES)
	$(TKL_COMPILE) $(MAINXML)

metaproxy.pdf: $(XMLFILES) multi.pdf
	for i in $(PNGFILES) common/id.png multi.pdf; do \
		if test ! -f $$i; then cp $(srcdir)/$$i $$i; fi; \
	done
	jade -E14 -D $(srcdir) -d common/print.dsl -t tex $(srcdir)/common/xml.dcl $(MAINXML)
	rm -f metaproxy.tex
	mv book.tex metaproxy.tex
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null

manref.xml: $(XMLMAN) $(srcdir)/common/ref2dbinc.xsl
	rm -f manref.xml
	for i in $(XMLMAN); do \
		xsltproc $(srcdir)/common/ref2dbinc.xsl $(srcdir)/$$i | sed 1d >> manref.xml; \
	done

progref.xml: metaproxy.xml
	rm -f $@
	xsltproc $(srcdir)/common/ref2dbinc.xsl $? | sed 1d >> $@

multi.png: multi.svg
	inkscape --export-png=$@ --export-width=800 --export-area-drawing $?

multi.eps: multi.svg
	inkscape --export-eps=$@ --export-bbox-page $?

%.pdf: %.eps
	epstopdf -hires $?

clean-data-hook:
	rm -f [0-9]* *.bak

dist-hook:
	if test -f index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		cp $$p $(distdir); \
	done

install-data-hook:
	if test -f index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		$(docDATA_INSTALL) $$p $(DESTDIR)/$(docdir); \
	done
