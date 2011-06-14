<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" 
		xmlns="http://www.loc.gov/mods/v3"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
		xmlns:id="http://indexdata.com/xml/modsExtension/"
		xmlns:pz="http://www.indexdata.com/pazpar2/1.0"
		x-exclude-result-prefixes="mods">
  <xsl:output method="xml" indent="yes" />
  <!-- <xsl:template match="text()"/> -->

  <xsl:template match="/pz:record">
      <mods version="3.4"
	  xsi:schemaLocation="http://www.loc.gov/mods/v3
			      http://www.loc.gov/standards/mods/v3/mods-3-4.xsd">
      <originInfo>
	<dateIssued><xsl:value-of select="normalize-space(pz:metadata[@type='date'])"/></dateIssued>
	<publisher>$PUBLISHER</publisher>
	<issuance>$PUBTYPE/$DOCTYPE</issuance>
      </originInfo>
      <location>
	<url usage="primary"><xsl:value-of select="pz:metadata[@type='electronic-url']"/></url>
	<url access="preview">$THUMBURL</url>
      </location>
      <titleInfo>
	<title><xsl:value-of select="pz:metadata[@type='title']"/></title>
      </titleInfo>
      <xsl:for-each select="pz:metadata[@type='author']">
	<name type="personal">
	  <displayForm>
	    <xsl:value-of select="."/>
	  </displayForm>
	  <role>
	    <roleTerm type="text">author</roleTerm>
	  </role>
	</name>
      </xsl:for-each>
      <abstract type="description">$DESCRIPTION</abstract>
      <subject>
	<topic>$SUBJECT</topic>
      </subject>
      <id:relevance>$RELEVANCE</id:relevance>
      <!-- <location> is repeatable for multiple holdings -->
      <location>
	<holdingSimple>
	  <copyInformation>
	    <subLocation>$LOCATION</subLocation>
	    <shelfLocator>$CALLNO</shelfLocator>
	    <id:circ>
	      <id:available>$AVAILABLE</id:available>
	      <id:due>$DUE</id:due>
	    </id:circ>
	  </copyInformation>
	</holdingSimple>
      </location>
      <relatedItem type="host">
	<titleInfo>
	  <title><xsl:value-of select="pz:metadata[@type='journal-title']"/></title>
	  <!-- or -->
	  <title>$BOOKTITLE</title>
	</titleInfo>
	<part>
	  <detail type="volume">
	    <number><xsl:value-of select="pz:metadata[@type='volume']"/></number>
	  </detail>
	  <detail type="issue">
	    <number>$ISSUE</number>
	  </detail>
	  <extent unit="pages">
	    <start>$STARTPAGE</start>
	    <end>$ENDPAGE</end>
	  </extent>
	</part>
      </relatedItem>
      <physicalDescription>
	<form>$MEDIUM</form>
	<internetMediaType>$FORMAT</internetMediaType>
	<extent>$EXTENT</extent>
      </physicalDescription>
      <id:citation>$CITATION</id:citation>
      <identifier type="issn">$ISSN</identifier>
      <identifier type="isbn">$ISBN</identifier>
      <identifier>$ID</identifier>
      <accessCondition type="copyright">$COPYRIGHT</accessCondition>
      <accessCondition type="copyrightabstract">$COPYRIGHTABSTRACT</accessCondition>
      <language usage="primary">
	<languageTerm type="text">$LANGUAGEITEM</languageTerm>
      </language>
      <language objectPart="summary">
	<languageTerm type="text">$LANGUAGEABSTRACT</languageTerm>
      </language>
    </mods>
  </xsl:template>
</xsl:stylesheet>
