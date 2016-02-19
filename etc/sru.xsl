<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:srw="http://www.loc.gov/zing/srw/"
                xmlns:dc="http://www.loc.gov/zing/srw/dcschema/v1.0/"
                xmlns:zr="http://explain.z3950.org/dtd/2.0/"
                xmlns:diag="http://www.loc.gov/zing/srw/diagnostic/"
                version="1.0">

  <xsl:output method="html" version="1.0" encoding="UTF-8" indent="yes"/>

  <xsl:template match="text()"/>

  <xsl:template match="/">
    <xsl:call-template name="html"/>
  </xsl:template>

  <xsl:template name="html">
    <html>
      <head>
        <title>
          <xsl:value-of select="//zr:explain/zr:databaseInfo/zr:title"/>
        </title>
        <link href="css.css" rel="stylesheet"
              type="text/css" media="screen, all"/>
      </head>
      <body>
        <div class="body">
          <xsl:apply-templates/>
        </div>
      </body>
    </html>
  </xsl:template>

  <!-- explain -->
  <xsl:template match="//zr:explain">
    <xsl:call-template name="dbinfo"/>
    <xsl:call-template name="diagnostic"/>
    <xsl:call-template name="indexinfo"/>
    <xsl:call-template name="relationinfo"/>
    <xsl:call-template name="searchform"/>
    <xsl:call-template name="scanform"/>
  </xsl:template>


  <!-- searchRetrieveResponse -->
  <xsl:template match="//srw:searchRetrieveResponse">
    <h2>Search Results</h2>
    <xsl:call-template name="diagnostic"/>
    <xsl:call-template name="displaysearch"/>
  </xsl:template>

  <!-- scanResponse -->
  <xsl:template match="//srw:scanResponse">
    <h2>Scan Results</h2>
    <xsl:call-template name="diagnostic"/>
    <xsl:call-template name="displayscan"/>
  </xsl:template>


  <xsl:template name="dbinfo">
    <div class="dbinfo">
      <h1><xsl:value-of select="//zr:explain/zr:databaseInfo/zr:title"/>
      </h1>
      <h2><xsl:value-of select="//zr:explain/zr:databaseInfo/zr:description"/>
      </h2>
      <h4>
        <xsl:value-of select="//zr:explain/zr:databaseInfo/zr:author"/>
        <br/>
        <xsl:value-of select="//zr:explain/zr:databaseInfo/zr:history"/>
      </h4>
    </div>
  </xsl:template>


  <xsl:template name="searchform">
    <div class="searchform">
      <form name="searchform"  method="get"> <!-- action=".." -->
        <input type="hidden" name="version" value="1.1"/>
        <input type="hidden" name="operation" value="searchRetrieve"/>
        <div class="query">
          <xsl:text>Search query: </xsl:text>
          <input type="text" name="query"/>
        </div>
        <div class="parameters">
          <xsl:text>startRecord: </xsl:text>
          <input type="text" name="startRecord" value="1"/>
          <xsl:text> maximumRecords: </xsl:text>
          <input type="text" name="maximumRecords" value="0"/>
          <xsl:text> recordSchema: </xsl:text>
          <select name="recordSchema">
          <xsl:for-each select="//zr:schemaInfo/zr:schema">
            <option value="{@name}">
              <xsl:value-of select="zr:title"/>
            </option>
          </xsl:for-each>
          </select>
          <xsl:text> recordPacking: </xsl:text>
          <select name="recordPacking">
            <option value="string">string</option>
            <option value="xml">XML</option>
          </select>
          <xsl:text> stylesheet: </xsl:text>
          <select name="stylesheet">
            <option value="/etc/sru.xsl">SRU</option>
            <option value="">NONE</option>
          </select>
        </div>

        <div class="submit">
          <input type="submit" value="Send Search Request"/>
        </div>
      </form>
    </div>
  </xsl:template>

  <xsl:template name="scanform">
    <div class="scanform">
      <form name="scanform" method="get"> <!-- action=".." -->
        <input type="hidden" name="version" value="1.1"/>
        <input type="hidden" name="operation" value="scan"/>
        <div class="scanClause">
          <xsl:text>Scan clause: </xsl:text>
          <input type="text" name="scanClause"/>
        </div>
        <div class="parameters">
          <xsl:text>responsePosition: </xsl:text>
          <input type="text" name="responsePosition" value="1"/>
          <xsl:text> maximumTerms: </xsl:text>
          <input type="text" name="maximumTerms" value="15"/>
          <xsl:text> stylesheet: </xsl:text>
          <select name="stylesheet">
            <option value="/etc/sru.xsl">SRU</option>
            <option value="">NONE</option>
          </select>
        </div>

        <div class="submit">
          <input type="submit" value="Send Scan Request"/>
        </div>
      </form>
    </div>
  </xsl:template>


  <xsl:template name="indexinfo">
     <div class="dbinfo">
       <xsl:for-each
          select="//zr:indexInfo/zr:index[zr:map/zr:name/@set]">
        <xsl:variable name="index">
          <xsl:value-of select="zr:map/zr:name/@set"/>
          <xsl:text>.</xsl:text>
          <xsl:value-of select="zr:map/zr:name/text()"/>
        </xsl:variable>
        <b><xsl:value-of select="$index"/></b><br/>
      </xsl:for-each>
     </div>
  </xsl:template>


  <xsl:template name="relationinfo">
    <!--
      <xsl:variable name="defrel"
                    select="//zr:configInfo/zr:default[@type='relation']"/>
      <b><xsl:value-of select="$defrel"/></b><br/>
      -->
      <xsl:for-each select="//zr:configInfo/zr:supports[@type='relation']">
        <xsl:variable name="rel" select="text()"/>
        <b><xsl:value-of select="$rel"/></b><br/>
      </xsl:for-each>
  </xsl:template>


  <!-- diagnostics -->
  <xsl:template name="diagnostic">
    <xsl:for-each select="//diag:diagnostic">
     <div class="diagnostic">
        <!-- <xsl:value-of select="diag:uri"/> -->
        <xsl:text> </xsl:text>
        <xsl:value-of select="diag:message"/>
        <xsl:text>: </xsl:text>
        <xsl:value-of select="diag:details"/>
      </div>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="displaysearch">
    <div class="searchresults">
      <xsl:for-each select="srw:numberOfRecords">
        <h4>
          <xsl:text>Number of Records: </xsl:text>
          <xsl:value-of select="."/>
        </h4>
      </xsl:for-each>
      <xsl:for-each select="srw:nextRecordPosition">
        <h4>
          <xsl:text>Next Record Position: </xsl:text>
          <xsl:value-of select="."/>
         </h4>
      </xsl:for-each>

      <xsl:for-each select="srw:records">
        <xsl:for-each select="srw:record">
          <div class="record">
            <p>
              <xsl:text>Record: </xsl:text>
              <xsl:value-of select="srw:recordPosition"/>
              <xsl:text> : </xsl:text>
              <xsl:value-of select="srw:recordSchema"/>
              <xsl:text> : </xsl:text>
              <xsl:value-of select="srw:recordPacking"/>
            </p>
            <p>
              <pre>
                <xsl:value-of select="srw:recordData"/>
              </pre>
            </p>
          </div>
        </xsl:for-each>
      </xsl:for-each>
    </div>
  </xsl:template>

  <xsl:template name="displayscan">
    <div class="scanresults">

      <xsl:for-each select="srw:terms">
        <xsl:for-each select="srw:term">
          <div class="term">

              <!-- <xsl:text>Term: </xsl:text> -->
              <xsl:for-each select="srw:displayTerm">
                <xsl:value-of select="."/>
                <xsl:text> : </xsl:text>
              </xsl:for-each>

              <xsl:for-each select="srw:value">
                <xsl:value-of select="."/>
              </xsl:for-each>

              <xsl:for-each select="srw:numberOfRecords">
                <xsl:text> (</xsl:text>
                <xsl:value-of select="."/>
                <xsl:text>)</xsl:text>
              </xsl:for-each>

              <xsl:for-each select="srw:extraTermData">
                <xsl:text> - </xsl:text>
                <xsl:value-of select="."/>
              </xsl:for-each>

          </div>
        </xsl:for-each>
      </xsl:for-each>

    </div>
  </xsl:template>


</xsl:stylesheet>
