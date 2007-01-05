## $Id: Makefile.am,v 1.34 2007-01-05 10:56:17 marc Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUBDIRS = common

SUFFIXES=.3mp .1 .pdf .tkl .esp .xml 

XMLFILES = book.xml manref.xml copyright.xml

MAINXML = $(srcdir)/book.xml

XMLMAN = metaproxy.xml \
	auth_simple.xml \
	backend_test.xml \
	bounce.xml \
	frontend_net.xml \
	http_file.xml \
	load_balance.xml \
	log.xml \
	multi.xml \
	query_rewrite.xml \
        record_transform.xml\
	session_shared.xml \
        sru_z3950.xml\
	template.xml \
	virt_db.xml \
	z3950_client.xml \
        zeerex_explain.xml

MANFILES = metaproxy.1 \
	auth_simple.3mp \
	backend_test.3mp  \
	bounce.3mp \
        frontend_net.3mp \
	http_file.3mp \
	load_balance.3mp \
	log.3mp \
	multi.3mp query_rewrite.3mp \
        record_transform.3mp\
	session_shared.3mp \
        sru_z3950.xml\
	template.3mp \
	virt_db.3mp \
	z3950_client.3mp \
	zeerex_explain.3mp


HTMLFILES = index.html

# testing if this does the job of collecting up the unnamed '*.html' files 
#UNLISTEDHTML = $(wildcard *.html)

PNGFILES = multi.png

doc_DATA = $(HTMLFILES) $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(XMLMAN) $(doc_DATA) $(man_MANS) multi.svg 

$(HTMLFILES): $(XMLFILES) $(PNGFILES)
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

manref.xml: $(XMLMAN) $(srcdir)/common/stripref.xsl
	rm -f manref.xml
	for i in $(XMLMAN); do \
		xsltproc $(srcdir)/common/stripref.xsl $(srcdir)/$$i | sed 1d >> manref.xml; \
	done

multi.png: multi.svg
	inkscape --export-png=$@ --export-width=800 --export-area-drawing $?

multi.eps: multi.svg
	inkscape --export-eps=$@ --export-bbox-page $?

.eps.pdf:
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

uninstall-hook:
	rm -r $(DESTDIR)/$(docdir)

