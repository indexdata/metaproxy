## $Id: Makefile.am,v 1.1 2006-02-02 18:22:47 mike Exp $
docdir=$(datadir)/doc/@PACKAGE@

SUPPORTFILES = \
 yp2html.dsl \
 yp2php.dsl \
 yp2print.dsl \
 tkl.xsl \
 xml.dcl

XMLFILES = yp2.xml.in

HTMLFILES = yp2.html 



PNGFILES=
EPSFILES=

REFFILES= 

doc_DATA = $(HTMLFILES) yp2.pdf $(PNGFILES)
man_MANS = $(MANFILES)

EXTRA_DIST = $(XMLFILES) $(SUPPORTFILES) $(REFFILES) \
	$(doc_DATA) $(EPSFILES) $(man_MANS) $(REFFILES)

$(HTMLFILES): $(XMLFILES)
	jade -E14 -D $(srcdir) -d yp2html.dsl -t sgml $(srcdir)/xml.dcl yp2.xml

yp2.php: $(XMLFILES)
	jade -E14 -D $(srcdir) -d yp2php.dsl -t sgml $(srcdir)/xml.dcl yp2.xml

yp2.pdf: $(XMLFILES)
	for i in $(PNGFILES); do \
		if test ! -f $$i; then ln -s $(srcdir)/$$i .; fi; \
	done
	jade -E14 -D $(srcdir) -d yp2print.dsl -t tex $(srcdir)/xml.dcl yp2.xml
	pdfjadetex yp2.tex >/dev/null
	pdfjadetex yp2.tex >/dev/null
	pdfjadetex yp2.tex >/dev/null

index.tkl: $(XMLFILES) tkl.xsl
	xsltproc tkl.xsl yp2.xml

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
