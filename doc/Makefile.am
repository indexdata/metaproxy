## $Id: Makefile.am,v 1.2 2006-03-16 12:03:39 adam Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUPPORTFILES = html.dsl print.dsl tkl.xsl xml.dcl

XMLFILES = metaproxy.xml.in book.xml
MAINXML = metaproxy.xml

HTMLFILES = metaproxy.html 

PNGFILES=
EPSFILES=

REFFILES= 

doc_DATA = $(HTMLFILES) metaproxy.pdf $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(SUPPORTFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES)

$(HTMLFILES): $(XMLFILES)
	jade -E14 -D $(srcdir) -d html.dsl -t sgml $(srcdir)/xml.dcl $(MAINXML)

metaproxy.pdf: $(XMLFILES)
	for i in $(PNGFILES); do \
		if test ! -f $$i; then ln -s $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d print.dsl -t tex $(srcdir)/xml.dcl $(MAINXML)
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null
	pdfjadetex metaproxy.tex >/dev/null

index.tkl: $(XMLFILES) tkl.xsl
	xsltproc tkl.xsl $(MAINXML)

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
