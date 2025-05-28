from lxml import etree

#解析XML和XSLT文件

xml_doc = etree.parse('book.xml')


xslt_doc = etree.parse('names.xslt')


#创建转换器

transform = etree.XSLT(xslt_doc)


#进行转换

html_doc = transform(xml_doc)


#输出结果

print(str(html_doc))


