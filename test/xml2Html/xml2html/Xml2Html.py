# -*- coding: utf-8 -*-

from lxml import etree

#解析XML

xml_doc = etree.parse('VW381040010_20250321T201935.xml')


xslt_doc = etree.parse('template.xslt')


#创建转换器

transform = etree.XSLT(xslt_doc)


#进行转换
html_doc = transform(xml_doc)


#输出结果
#输出结果
with  open('XmltoHtml.html', 'w', encoding="utf-8") as f1:
    f1.write(str(html_doc))
    f1.close()

print(str(html_doc))


