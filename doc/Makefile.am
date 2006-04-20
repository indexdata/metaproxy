## $Id: Makefile.am,v 1.7 2006-04-20 09:28:24 mike Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUFFIXES=.3mp .1

SUPPORTFILES = html.dsl print.dsl tkl.xsl xml.dcl ref2dbinc.xsl

XMLFILES = main.xml.in book.xml manref.xml progref.xml copyright.xml
MAINXML = main.xml
HTMLFILES = metaproxy.html 

XMLMAN   = auth_simple.xml backend_test.xml frontend_net.xml \
	http_file.xml log.xml multi.xml query_rewrite.xml \
	session_shared.xml template.xml virt_db.xml z3950_client.xml
MANFILES = auth_simple.3mp backend_test.3mp frontend_net.3mp \
	http_file.3mp log.3mp multi.3mp query_rewrite.3mp \
	session_shared.3mp template.3mp virt_db.3mp z3950_client.3mp \
	metaproxy.1

PNGFILES=
EPSFILES=

REFFILES= 

doc_DATA = $(HTMLFILES) metaproxy.pdf $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(SUPPORTFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES)

$(HTMLFILES): $(XMLFILES)
	jade -E14 -D $(srcdir) -d html.dsl -t sgml $(srcdir)/xml.dcl $(MAINXML)

.xml.3mp:
	docbook2man $<

.xml.1:
	docbook2man $<

metaproxy.pdf: $(XMLFILES)
	for i in $(PNGFILES); do \
		if test ! -f $$i; then ln -s $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d print.dsl -t tex $(srcdir)/xml.dcl $(MAINXML)
	rm -f metaproxy.tex;
	mv main.tex metaproxy.tex
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null

index.tkl: $(XMLFILES) tkl.xsl
	xsltproc tkl.xsl $(MAINXML)

manref.xml: $(XMLMAN) ref2dbinc.xsl
	rm -f manref.xml
	for i in $(XMLMAN); do \
		xsltproc ref2dbinc.xsl $$i | sed 1d >> manref.xml; \
	done

progref.xml: metaproxy.xml
	rm -f $@
	xsltproc ref2dbinc.xsl $? | sed 1d >> $@

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
