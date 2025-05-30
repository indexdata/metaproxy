SUBDIRS = common

SUFFIXES=.3mp .1 .pdf .tkl .esp .xml

XMLFILES = book.xml manref.xml copyright.xml gpl-2.0.xml local.ent

MAINXML = $(srcdir)/book.xml

XMLMAN = metaproxy.xml \
	metaproxy-config.xml \
	auth_simple.xml \
	backend_test.xml \
	bounce.xml \
	cgi.xml \
        cql_rpn.xml \
	frontend_net.xml \
	http_client.xml \
	http_file.xml \
	http_rewrite.xml \
	limit.xml \
	load_balance.xml \
	log.xml \
	multi.xml \
	present_chunk.xml \
	query_rewrite.xml \
	record_transform.xml\
	sd_remove.xml \
	session_shared.xml \
	sort.xml \
        sru_z3950.xml\
	template.xml \
	virt_db.xml \
	z3950_client.xml \
        zeerex_explain.xml \
	zoom.xml

MANFILES = metaproxy.1 \
	metaproxy-config.1 \
	auth_simple.3mp \
	backend_test.3mp  \
	bounce.3mp \
	cgi.3mp \
        cql_rpn.3mp \
        frontend_net.3mp \
	http_client.3mp \
	http_file.3mp \
	http_rewrite.3mp \
	limit.3mp \
	load_balance.3mp \
	log.3mp \
	multi.3mp \
	present_chunk.3mp \
	query_rewrite.3mp \
        record_transform.3mp \
	sd_remove.3mp \
	session_shared.3mp \
	sort.3mp \
        sru_z3950.3mp \
	template.3mp \
	virt_db.3mp \
	z3950_client.3mp \
	zeerex_explain.3mp \
	zoom.3mp

HTMLFILES = index.html

# testing if this does the job of collecting up the unnamed '*.html' files
#UNLISTEDHTML = $(wildcard *.html)

PNGFILES = multi.png

doc_DATA = $(HTMLFILES) $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(XMLMAN) $(doc_DATA) $(man_MANS) multi.svg

$(MANFILES): local.ent

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
	$(PDF_COMPILE) $(MAINXML)
	mv book.pdf metaproxy.pdf

metaproxyj.pdf: $(XMLFILES) multi.pdf
	for i in $(PNGFILES) common/id.png multi.pdf; do \
		if test ! -f $$i; then cp $(srcdir)/$$i $$i; fi; \
	done
	jade  -E14 -D $(srcdir) -d common/print.dsl -t tex $(srcdir)/common/xml.dcl $(MAINXML)
	pdfjadetex book.tex
	pdfjadetex book.tex
	pdfjadetex book.tex
	mv book.pdf metaproxyj.pdf

manref.xml: $(XMLMAN) $(srcdir)/common/stripref.xsl \
		$(srcdir)/copyright.xml local.ent
	rm -f manref.xml
	for i in $(XMLMAN); do \
		xsltproc --xinclude $(srcdir)/common/stripref.xsl $(srcdir)/$$i | sed 1d >> manref.xml; \
	done


multi.png: multi.svg
	rsvg-convert -w 1200 -o $@ $?

multi.pdf: multi.svg
	rsvg-convert -f pdf -o $@ $?

clean-data-hook:
	rm -f [0-9]* *.bak

dist-hook:
	if test -f index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		cp $$p $(distdir); \
	done

doc-clean:
	rm -f manref.xml *.html *.[0-9] *.?mp *.pdf toc.hhc htmlhelp.hhp local.ent

install-data-hook:
	if test -f index.html; then d=.; else d="$(srcdir)"; fi; \
	for p in $$d/*.html; do \
		$(INSTALL_DATA) $$p $(DESTDIR)$(docdir); \
	done

uninstall-hook:
	rm -r $(DESTDIR)$(docdir)

distclean-local: doc-clean
