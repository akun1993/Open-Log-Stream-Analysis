<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  


  <xsl:param name="repeatCount" select="3"/>
  
  <xsl:template match="/">
    
    <xsl:call-template name="repeatText">
      <xsl:with-param name="text" select="'Hello'"/>
    </xsl:call-template>
  </xsl:template>
  
  <xsl:template name="repeatText">
    <xsl:param name="text"/>
    <xsl:param name="count" select="1"/>
    <xsl:if test="$count &lt;= $repeatCount">
      <xsl:value-of select="$text"/>
      <xsl:text> </xsl:text> <!-- 添加空格分隔 -->
      <xsl:call-template name="repeatText">
        <xsl:with-param name="text" select="$text"/>
        <xsl:with-param name="count" select="$count + 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>