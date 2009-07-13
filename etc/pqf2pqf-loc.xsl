<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output indent="yes" method="xml" version="1.0" encoding="UTF-8"/>

  <!--
./yaz-xmlquery -p '@and @attr 1=1016 @attr 4=2 @attr 6=3 the @attr 1=4 fish' > test.xml && xmllint -format test.xml && ./yaz-xmlquery -x test1.xml && xsltproc pqf2pqf.xsl test.xml |tee test2.xml && ./yaz-xmlquery -x test2.xml 

./yaz-xmlquery -p '@not @attr 1=1016 @attr 4=2 @attr 6=3 @attr 7=1 @attr 8=4 fish @attr 1=4 fish' > test.xml && xmllint -format test.xml && ./yaz-xmlquery -x test.xml && xsltproc pqf2pqf.xsl test.xml |tee test2.xml && ./yaz-xmlquery -x test2.xml 
  -->

  <!-- disable default templates -->
  <xsl:template match="text()"/>
  <xsl:template match="node()"/>

  <!-- identity stylesheet templates -->
  <!-- these parse pqf-xml input recursively and make identity operations -->
  <xsl:template match="/query">
    <query>
      <xsl:apply-templates/>
    </query>
  </xsl:template>

  <xsl:template match="rpn">
    <rpn>
      <xsl:attribute name="set">
        <xsl:value-of  select="@set"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </rpn>
  </xsl:template>

  <xsl:template match="operator">
    <operator>
      <xsl:attribute name="type">
        <xsl:value-of  select="@type"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </operator>
  </xsl:template>

  <xsl:template match="apt">
    <apt>
      <xsl:apply-templates select="attr"/>
      <xsl:apply-templates select="term"/>
    </apt>
  </xsl:template>

  <xsl:template match="attr">
    <xsl:copy-of select="."/>
  </xsl:template>

  <xsl:template match="term">
    <xsl:copy-of select="."/>
  </xsl:template>

  <!-- validate use attributes -->
  <xsl:template match="attr[@type=1]">
    <xsl:choose>
       <xsl:when test="@value &gt;= 1 and @value &lt;= 11">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:when test="@value &gt;= 13 and @value &lt;= 1010">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:when test="@value &gt;= 1013 and @value &lt;= 1023">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:when test="@value &gt;= 1025 and @value &lt;= 1030">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:otherwise>
         <diagnostic code="114" addinfo="{@value}"/>
       </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- validate relation attributes -->
  <xsl:template match="attr[@type=2]">
    <xsl:choose>
       <xsl:when test="@value &gt;= 1 and @value &lt;= 6">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:otherwise>
         <diagnostic code="117" addinfo="{@value}"/>
       </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- validate position attributes -->
  <xsl:template match="attr[@type=3]">
    <xsl:choose>
       <xsl:when test="@value &gt;= 1 and @value &lt;= 3">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:otherwise>
         <diagnostic code="119" addinfo="{@value}"/>
       </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- validate structure attributes -->
  <xsl:template match="attr[@type=4]">
    <xsl:choose>
       <xsl:when test="@value &gt;= 1 and @value &lt;= 6">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:otherwise>
         <diagnostic code="118" addinfo="{@value}"/>
       </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- validate truncation attributes -->
  <xsl:template match="attr[@type=5]">
    <xsl:choose>
       <xsl:when test="@value = 1 or @value = 100">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:otherwise>
         <diagnostic code="120" addinfo="{@value}"/>
       </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- validate completeness attributes -->
  <xsl:template match="attr[@type=6]">
    <xsl:choose>
       <xsl:when test="@value &gt;= 1 and @value &lt;= 3">
         <xsl:copy-of select="."/>
       </xsl:when>
       <xsl:otherwise>
         <diagnostic code="122" addinfo="{@value}"/>
       </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- throw error for other types -->
  <xsl:template match="attr[@type &gt;= 7]">
     <diagnostic code="113" addinfo="{@type}"/>
  </xsl:template>

</xsl:stylesheet>


