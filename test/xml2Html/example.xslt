
<!-- 动态赋值 -->
<xsl:with-param name="dynamicVal" select="book/price"/>
<!-- 静态赋值 -->
<xsl:with-param name="staticVal">Default Text</xsl:with-param>

<xsl:apply-templates select="name|weapon"/> <!-- 并行处理多个子节点 -->

<xsl:template match="para">
  <xsl:call-template name="information">
    <!-- Content:xsl:with-param* -->
  </xsl:call-template>
  <p><xsl:apply-templates/></p>
</xsl:template>

<!-- 方法2：使用<xsl:text>和CDATA区块
对于较早的XSLT版本，你可以使用<xsl:text>和CDATA区块来包含JavaScript代码： -->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
      <xsl:output method="html" indent="yes"/>
      
      <xsl:template match="/">
        <html>
          <head>
            <title>示例页面</title>
            <script type="text/javascript">
              <xsl:text disable-output-escaping="yes">
            <![CDATA[
            function sayHello() {
              alert('Hello, World!');
            }
            ]]>
          </xsl:text>
            </script>
          </head>
          <body>
            <h1>欢迎来到我的页面</h1>
            <button onclick="sayHello()">点击我</button>
          </body>
        </html>
      </xsl:template>
    </xsl:stylesheet>

<xsl:result-document>
  
  
  如果你使用的是XSLT 2.0或更高版本，你可以使用<xsl:result-document>来生成一个包含JavaScript的HTML文件。例如：
    
    <xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
      <xsl:output method="html" indent="yes"/>
      
      <xsl:template match="/">
        <xsl:result-document href="output.html" method="html">
          <html>
            <head>
              <title>示例页面</title>
              <script type="text/javascript">
                function sayHello() {
                alert('Hello, World!');
                }
              </script>
            </head>
            <body>
              <h1>欢迎来到我的页面</h1>
              <button onclick="sayHello()">点击我</button>
            </body>
          </html>
        </xsl:result-document>
      </xsl:template>
    </xsl:stylesheet>
