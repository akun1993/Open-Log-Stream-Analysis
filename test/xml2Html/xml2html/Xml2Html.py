# -*- coding: utf-8 -*-
import time
from lxml import etree

#解析XML

xml_doc = etree.parse('Tbox-Report-1754554071.xml')


xslt_doc = etree.parse('template.xslt')


#创建转换器

transform = etree.XSLT(xslt_doc)


#进行转换
html_doc = transform(xml_doc)


#输出结果
#输出结果

timestamp = time.time()
outfile = "XmltoHtml-" + str(int(timestamp)) + ".html"

with  open(outfile, 'w', encoding="utf-8") as f1:
    f1.write(str(html_doc))
    f1.close()

print(str(html_doc))


