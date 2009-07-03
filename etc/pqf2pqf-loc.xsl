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

  <!-- throw diagnostic for unsupported use attributes -->
  <xsl:template match="attr[@type=1][@value=12]">
    <diagnostic code="114" addinfo="{@value}"/>
  </xsl:template>

</xsl:stylesheet>


