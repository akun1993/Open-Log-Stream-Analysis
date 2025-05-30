<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="xml" indent="yes"/>
 
  <xsl:template match="*">
    <xsl:copy>
      <xsl:attribute name="depth">
        <xsl:value-of select="count(ancestor::*) + 1"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>
 
  <!-- 匹配文本节点和其他不需要处理的节点 -->
  <xsl:template match="text()|@*">
    <xsl:copy/>
  </xsl:template>
</xsl:stylesheet>