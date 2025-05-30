<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="xml" indent="yes"/>
  
  <!-- 递归模板，计算深度 -->
  <xsl:template match="*">
    <xsl:param name="depth" select="1"/>
    <xsl:copy>
      <xsl:attribute name="depth">
        <xsl:value-of select="$depth"/>
      </xsl:attribute>
      <xsl:apply-templates>
        <xsl:with-param name="depth" select="$depth + 1"/>
      </xsl:apply-templates>
    </xsl:copy>
  </xsl:template>
  
  <!-- 匹配文本节点和其他不需要处理的节点 -->
  <xsl:template match="text()|@*">
    <xsl:copy/>
  </xsl:template>
</xsl:stylesheet>