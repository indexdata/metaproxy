## $Id: Makefile.am,v 1.15 2006-04-27 16:54:39 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUBDIRS = common

SUFFIXES=.3mp .1

XMLFILES = main.xml.in book.xml manref.xml progref.xml copyright.xml \
	metaproxy.xml

MAINXML = main.xml

XMLMAN   = auth_simple.xml backend_test.xml frontend_net.xml \
	http_file.xml log.xml multi.xml query_rewrite.xml \
	session_shared.xml template.xml virt_db.xml z3950_client.xml
MANFILES = auth_simple.3mp backend_test.3mp frontend_net.3mp \
	http_file.3mp log.3mp multi.3mp query_rewrite.3mp \
	session_shared.3mp template.3mp virt_db.3mp z3950_client.3mp \
	metaproxy.1

HTMLFILES = architecture.html \
	classes.html \
	configuration.html \
	example.configuration.html \
	extensions.html \
	filterref.html \
	filters.html \
	future.directions.html \
	individual.classes.html \
	installation.html \
	installation.debian.html \
	installation.windows.html \
	introduction.html \
	licence.html \
	metaproxy.html \
	multidb.html \
	multidb.multi.html \
	multidb.picture.html \
	multidb.virt_db.html \
	multidb.what.html \
	other.source.files.html \
	overview.filter.types.html \
	overview.xml.structure.html \
	refguide.html

PNGFILES=multi.png

doc_DATA = $(HTMLFILES) metaproxy.pdf $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(XMLMAN) $(doc_DATA) $(man_MANS) multi.svg

$(HTMLFILES): $(XMLFILES) multi.png
	jade -E14 -D $(srcdir) -d common/html.dsl -t sgml $(srcdir)/common/xml.dcl $(MAINXML)

.xml.3mp:
	docbook2man $<

.xml.1:
	docbook2man $<

metaproxy.pdf: $(XMLFILES) multi.pdf
	for i in $(PNGFILES) multi.pdf; do \
		if test ! -f $$i; then ln -s $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d common/print.dsl -t tex $(srcdir)/common/xml.dcl $(MAINXML)
	rm -f metaproxy.tex
	mv main.tex metaproxy.tex
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null

index.tkl: $(XMLFILES) common/tkl.xsl
	xsltproc common/tkl.xsl $(MAINXML)

manref.xml: $(XMLMAN) $(srcdir)/common/ref2dbinc.xsl
	rm -f manref.xml
	for i in $(XMLMAN); do \
		xsltproc $(srcdir)/common/ref2dbinc.xsl $$i | sed 1d >> manref.xml; \
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
	for f in $(srcdir)/*.html; do \
		found=0; \
		b=`basename $$f`; \
		for h in $(HTMLFILES); do \
			if test "$$h" = "$$b"; then \
				found=1; \
			fi \
		done; \
		if test "$$found" = "0"; then \
			echo "$$f not found in HTMLFILES"; \
			exit 1; \
		fi \
	done
