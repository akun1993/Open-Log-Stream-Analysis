
#include "tinyxml2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>


int main(int argc,char **argv){
    //
	   // 创建一个 XMLDocument 对象
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLDeclaration* decl = doc.NewDeclaration("xml version='1.0' encoding='UTF-8' standalone='yes'");
    doc.InsertFirstChild(decl);

    tinyxml2::XMLDeclaration* decl2 = doc.NewDeclaration("xml-stylesheet type=\"text/xsl\" href=\"Anything.xsl\"");
    doc.InsertAfterChild(decl,decl2);

    // 创建一个根节点 <Root>
    tinyxml2::XMLElement* root = doc.NewElement("protocol");
    doc.InsertEndChild(root);
    
    // 创建一个子节点 <Element> 并设置属性 attribute="value"
    tinyxml2::XMLElement* info = doc.NewElement("information");

    tinyxml2::XMLElement* soc_ver = doc.NewElement("soc_version");
    tinyxml2::XMLText* ver_text = doc.NewText("This is a text node");
    soc_ver->InsertEndChild(ver_text);

    info->InsertEndChild(soc_ver);

    root->InsertEndChild(info);
    

    
    // 创建另一个子节点 <SubElement> 并添加文本和属性
    tinyxml2::XMLElement* subElement = doc.NewElement("SubElement");
    subElement->SetAttribute("type","fs");

    root->InsertEndChild(subElement);
    tinyxml2::XMLText* subText = doc.NewText("This is a sub text node");
    subElement->InsertEndChild(subText);
    
    // 保存文件到磁盘
    tinyxml2::XMLError eResult = doc.SaveFile("example.xml");
    if (eResult != tinyxml2::XML_SUCCESS) {
        //std::cerr << "Error saving file: " << eResult << std::endl;
        //return -1;
    } else {
        //std::cout << "File saved successfully." << std::endl;
    }


    doc.Clear();


  
}

