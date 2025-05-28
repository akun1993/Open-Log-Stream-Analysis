<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
     <xsl:output method="html" version="1.0" encoding="utf-8" indent="yes"/>    
    <xsl:template match="students">
        <xsl:copy>
            <xsl:for-each select="student">
                <xsl:sort select="@id" data-type="number"/>
                <xsl:if test="generate-id(.)=generate-id(../student[@id=current()/@id])">
                    <xsl:copy>
                        <xsl:attribute name="id">
                            <xsl:value-of select="@id"/>
                        </xsl:attribute> 
                        <xsl:attribute name="name">
                            <xsl:value-of select="name"/>
                        </xsl:attribute>  
                        <xsl:for-each select="../student[@id=current()/@id]">
                            <xsl:sort select="mark/@course"/>
                            <xsl:copy-of select="mark"/>                        
                        </xsl:for-each>                  
                    </xsl:copy>                
                </xsl:if>            
            </xsl:for-each>        
        </xsl:copy>
    </xsl:template>
</xsl:stylesheet>