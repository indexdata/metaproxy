## $Id: Makefile.am,v 1.3 2006-03-16 13:20:04 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUPPORTFILES = html.dsl print.dsl tkl.xsl xml.dcl ref2dbinc.xsl

XMLFILES = main.xml.in book.xml manref.xml copyright.xml
MAINXML = main.xml
HTMLFILES = metaproxy.html 

XMLMAN=auth_simple.xml metaproxy.xml
MANFILES=auth_simple.3mp metaproxy.1

PNGFILES=
EPSFILES=

REFFILES= 

doc_DATA = $(HTMLFILES) metaproxy.pdf $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(SUPPORTFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES)

$(HTMLFILES): $(XMLFILES) manref.xml 
	jade -E14 -D $(srcdir) -d html.dsl -t sgml $(srcdir)/xml.dcl $(MAINXML)

auth_simple.3mp: auth_simple.xml
	docbook2man $<

metaproxy.1: metaproxy.xml
	docbook2man $<

metaproxy.pdf: $(XMLFILES) manref.xml
	for i in $(PNGFILES); do \
		if test ! -f $$i; then ln -s $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d print.dsl -t tex $(srcdir)/xml.dcl $(MAINXML)
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null

index.tkl: $(XMLFILES) tkl.xsl manref.xml
	xsltproc tkl.xsl $(MAINXML)

manref.xml: $(XMLMAN) ref2dbinc.xsl
	rm -f manref.xml
	for i in $(XMLMAN); do xsltproc ref2dbinc.xsl $$i >>manref.xml; done

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
