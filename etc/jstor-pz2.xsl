<?xml version="1.0" encoding="UTF-8"?>
<!--
    Converts JSTORs info:srw/schema/srw_jstor records to
    Pazpar2 records.
-->
<xsl:stylesheet
    version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:pz="http://www.indexdata.com/pazpar2/1.0"
    xmlns:jstor="http://dfr.jstor.org/sru/elements/1.1">

  <xsl:output indent="yes"
	      method="xml"
	      version="1.0"
	      encoding="UTF-8"/>
  
  <xsl:template match="/*">
    <pz:record>

      <xsl:for-each select="jstor:author">
	<pz:metadata type="author">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <!-- jstor:stringDate -->

      <xsl:for-each select="jstor:year">
	<xsl:if test='contains(.,"YEAR:")'>
	  <pz:metadata type="date">
	    <xsl:value-of select='substring-after(.,":")'/>
	  </pz:metadata>
	</xsl:if>
      </xsl:for-each>

      <xsl:for-each select="jstor:abstract">
	<pz:metadata type="description">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <!-- jstor:id -->

      <xsl:for-each select="jstor:issn">
	<pz:metadata type="issn">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <!-- haven't seen this one, actually -->
      <xsl:for-each select="jstor:isbn">
	<pz:metadata type="isbn">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <!-- jstor:lang -->

      <xsl:for-each select="jstor:publisher">
	<pz:metadata type="publisher">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <xsl:for-each select="jstor:topics">
	<pz:metadata type="subject">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <!-- jstor:disipline -->

      <xsl:for-each select="jstor:title">
	<pz:metadata type="title">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <xsl:for-each select="jstor:journaltitle">
	<pz:metadata type="journal-title">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <xsl:for-each select="jstor:volume">
	<pz:metadata type="volume-number">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <xsl:for-each select="jstor:issue">
	<pz:metadata type="issue-number">
	  <xsl:value-of select='.'/>
	</pz:metadata>
      </xsl:for-each>

      <!-- jstor:pagerange -->
      
      <!-- jstor:resourcetype -->

      <!-- jstor:type -->

    </pz:record>
  </xsl:template>

  <xsl:template match="text()"/>

</xsl:stylesheet>
