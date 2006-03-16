<?xml version="1.0"?>
<!-- $Id: ref2dbinc.xsl,v 1.1 2006-03-16 13:20:05 adam Exp $ -->
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

    <xsl:output method="xml" encoding="UTF-8"/>

<xsl:template match="/refentry">
  <section>
    <title>
      <xsl:value-of select="refmeta/refentrytitle"/>
    </title>
    <xsl:apply-templates/>
  </section>
</xsl:template>

<xsl:template match="refnamediv">
  <para>
     <xsl:value-of select="refpurpose"/> 
  </para>
</xsl:template>

<xsl:template match="refmeta">
</xsl:template>

<xsl:template match="refsynopsisdiv">
  <xsl:copy-of select="cmdsynopsis"/>
</xsl:template>

<xsl:template match="refsect1">
 <section>
  <xsl:copy-of select="*"/>
 </section>
</xsl:template>

</xsl:stylesheet>

