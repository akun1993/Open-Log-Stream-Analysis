
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="2.0">
    <xsl:output method="html" version="2.0" encoding="utf-8" indent="yes"/>

    <xsl:template match="/">

        <html>
    <head>
        <style type="text/css">
		html {direction:ltr;}
		table {border-width:0pt;border-color:black;border-style:solid;border-collapse:collapse;padding:0;margin:0;width:200mm;table-layout:auto}
		.border_style {border-width:1pt;border-color:black;border-style:solid;border-collapse:collapse;padding-left:5;padding-right:5;margin-top:10;margin-bottom:10;width:195mm}
		tr {border-width:0pt;border-color:black;border-style:solid;padding:0;margin:0;text-align:left}
		td {border-width:0pt;border-color:black;border-style:solid;padding:0;margin:0;text-align:left}
		.align_opposite {text-align:right;}
		span {margin-top:0pt;margin-bottom:0pt}
		div {margin-top:0pt;margin-bottom:10pt;margin-left:16pt}
		p {margin-top:0pt;margin-bottom:0pt}
		.p_mini {margin-top:0pt;margin-bottom:0pt}
		.p_small {margin-top:10pt;margin-bottom:10pt}
		.p_big {margin-top:16pt;margin-bottom:16pt}
		.courier_style {margin-top:0;margin-bottom:0;font-size:10pt;font-family:"Courier New";direction:ltr}
		.default_obd_mode {color:blue;margin-top:0;margin-bottom:0;font-size:10pt;font-weight:bold;font-family:Arial;direction:ltr}
		.default_error {color:red;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.default_error_b {color:red;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
		.default_warn {color:orange;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.default_warn_b {color:orange;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
		.default_ok {color:green;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.compstylenotequal {color:red;margin-top:0;margin-bottom:0;font-size:10pt;font-weight:bold;font-family:Arial;direction:ltr}
		.compstyleequal {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.default_style {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.default_style_revert_align {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:right}
		.default_style_topmargin {margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.default_style_b {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
		.default_style_b_indent {text-indent:20pt;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
		.default_style_bu {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;text-decoration:underline;direction:ltr}
		.default_style_bu_topmargin {margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;text-decoration:underline;direction:ltr}
		.default_style_bu_diagfunction {background-color:#C0C0C0;margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;text-decoration:underline;direction:ltr}
		.default_style_diagfunction {background-color:#C0C0C0;margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
		.default_style_ecu {background-color:#8FBCE6;margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
		.default_style_big_bi {margin-top:0;margin-bottom:0;font-size:16pt;font-family:Arial;font-weight:bold;font-style:italic;direction:ltr}
		.default_style_big_b {margin-top:0;margin-bottom:0;font-size:20pt;font-family:Arial;font-weight:bold;direction:ltr}
		.header_style_big {margin-top:0;margin-bottom:0;font-size:13pt;font-family:Arial;font-weight:bold;direction:ltr}
		.header_style_bigu {margin-top:0;margin-bottom:0;font-size:13pt;font-family:Arial;font-weight:bold;direction:ltr;text-decoration:underline}
		.header_style_small {margin-top:0;margin-bottom:0;font-size:11pt;font-family:Arial;direction:ltr}
		.small_style {margin-top:0;margin-bottom:0;font-size:8pt;font-family:Arial;direction:ltr}
		.very_small_style {margin-top:0;margin-bottom:0;font-size:6pt;font-family:Arial;direction:ltr}
		.small_style_b {margin-top:0;margin-bottom:0;font-size:8pt;font-family:Arial;font-weight:bold;direction:ltr}
		.small_style_bu {margin-top:0;margin-bottom:0;font-size:8pt;font-family:Arial;font-weight:bold;text-decoration:underline;direction:ltr}
		.font_arial {font-family:Arial}
		.font_courier {font-family:"Courier New"}
		.font_sidis {font-family:SidisSymbole}
		.font_symbol {font-family:"Symbol"}
		.font_wingdings {font-family:"Wingdings"}
		.size_small {font-size:10pt}
		.size_big {font-size:12pt}
		.color_black {color:#000000}
		.color_red {color:#FF0000}
		.color_green {color:#006400}
		.color_blue {color:#0000FF}
		.color_yellow {color:#FFF405}
		.color_green2 {color:#92C027}
		.color_blue2 {color:#6BBCE4}
		.style_normal {font-style:normal}
		.style_italic {font-style:italic}
		.weight_normal {font-weight:normal}
		.weight_bold {font-weight:bold}

		.table_spec {border-width:0;border-collapse:collapse;padding:0;margin:0;width:100%}
	 
		td.border_cell{border-width:1pt;border-color:black;border-style:solid;padding:0;margin:0;text-align:left}
		td.border_bottom{border-color:black;border-top:blank;border-bottom:solid;border-bottom-width:thin;border-right:blank;border-left:blank;padding:0;margin:0;text-align:left}
		table.small_table {border-width:0pt;border-color:black;border-style:solid;border-collapse:collapse;padding:0;margin:0;width:47mm;table-layout:auto;font-size:10pt;font-family:Arial}

		/* Styles for the expansion mechanism */
		input.treeView {float:left !important; z-index:-1 !important; position:absolute; vertical-align:middle;  width:0px; height:0px; overflow:hidden;}
		label.treeView.hover {text-decoration:underline; vertical-align:middle;}
		label.treeView {cursor:pointer; vertical-align:middle;}
		.treeView_label_content { position:relative; left:1.5em; vertical-align:middle;}
		.treeView_selector_item { position:absolute; display:table-cell; vertical-align:middle; border:1px solid; padding-left:0.33em; padding-right:0.33em; background-color:#E7E7E7; font-family:Consolas,'Courier New',Courier; margin-top:0;margin-bottom:0;font-size:10pt;font-weight:bold;direction:ltr}
		
			.treeView_content {clear:left; margin-left:2%; width:98%; background-color:#ffffff; display:none}
				
	
	 
		@media screen
		{
		  body { margin-top:0;margin-bottom:0 }
		  .table_spec2 {border-width:0;border-collapse:collapse;padding:0;margin:0;width:100%}
		  p {margin-top:0pt;margin-bottom:0pt }
		  .nichtdrucken {font-size:10pt;font-family:Arial;direction:ltr}
		}
	 
		#button_bar {position:fixed; right:15px; text-align:left;}
		#button_bar span {display:block;}

		@media print
		{
          body { margin-top:0;margin-bottom:0 }
          .table_spec2 {border-width:0;border-collapse:collapse;padding:0;margin:0;width:195mm}
          p {margin-top:0pt;margin-bottom:0pt }
          .nichtdrucken {font-size:10pt;font-family:Arial;direction:ltr;display:none}

		  /* do not print expansion mechanism items*/ 
		  .treeView_selector_item,
		  input.treeView { display:none; }
		  .treeView_label_content { position:static; }
		  #button_bar {display:none;}
		}
	  </style>

        <script type="text/javascript">
         <xsl:text disable-output-escaping="yes">
           <![CDATA[
		  function showHideSectionsAfterCheckbox(checkbox) {
			var startMarker = document.getElementById("start_marker_"+checkbox.id);
			showHideSectionByNode(startMarker.nextSibling, !(checkbox.checked));
		  }
		
		  function showSectionsByID(id) {
			var checkbox = document.getElementById("treeView_"+id);
			checkbox.checked=true;
			showHideSectionsAfterCheckbox(checkbox);
		  }
          
	 	  function set_all_treeViews(expand) {
			var input_elements = document.getElementsByName("treeView_checkbox");
			for (var i=0; i<input_elements.length; i++) {
			  if (expand) {
				if (!input_elements[i].checked) {
				  input_elements[i].checked=true;
				  showHideSectionsAfterCheckbox(input_elements[i]);
				}
			  }
			  else {
				if (input_elements[i].checked) {
				  input_elements[i].checked=false;
				  showHideSectionsAfterCheckbox(input_elements[i]);
				}
			  }
			}	
		  }

		  function expand_parent_node(nodeId) {
			var input_elements = document.getElementsByName(nodeId);
			expand_parent_node_recursivly(input_elements[0].parentNode);
		  }

		  function expand_parent_node_recursivly(node) {
			if (node.className == "treeView_content") {
			  checkChildCheckbox(node.previousSibling);
			  var currentNode = node.previousSibling;
			}
			else {
			  expand_parent_node_recursivly(node.parentNode);
			}
		  }
		  function checkChildCheckbox(node) {
			if (node.name == "treeView_checkbox") {
			  node.checked=true;
			  showHideSectionsAfterCheckbox(node);
			}
			else {
			  for (var i=0; i<node.childNodes.length; i++) {
				checkChildCheckbox(node.childNodes[i]);
			  }
			}
		  }
		
		  function showHideSectionByNode(node, hideBoolean) {
			for (var sibling=node; sibling!=null; sibling=sibling.nextSibling) {
			  with(sibling) {
				if (nodeType == 1) {
				  if (className == "treeView_content") {
					if (hideBoolean) {
					  style.display="none";
					}
					else {
					  //FireFox table display fix:
					  if (nodeName.toUpperCase() == "TBODY") {
						style.display="table-row-group";
					  }
					  else {
						style.display="block";
					  }
					}
				  } 
				  else if (className == "treeView") {
					if (nodeName.toLowerCase() == "input") {
					  if (type == "hidden") {
						return;
					  }
					  //Ignore the checkbox input itself
					}
					else if (nodeName.toLowerCase() == "label") {
					  for (var i=0; i<childNodes.length; i++) {
						with (childNodes[i]) {
						  if (className == "treeView_selector_item") {
							if (hideBoolean) {
							  firstChild.nodeValue="+";
							}
							else {
							  firstChild.nodeValue="-";
							}
							break;
						  }
						}
					  }
					}
				  } 
				  else {
					showHideSectionByNode(childNodes[0], hideBoolean);
				  }
				}
			  }
			}
		}
         ]]>
      </xsl:text>
	  </script>

    <title>Vehicle documentation</title>
    </head>

        <body>

        <xsl:apply-templates select="protocol/information"/> 
  
        <!-- Generate a list of picture titles, with each 
        title linking to the picture in the poem below. -->
        <b>Pictures:</b><br/>
        <xsl:for-each select="descendant::ecu">
            <a href="#{generate-id(ecu_id)}">
            <xsl:value-of select="ecu_name"/></a><br/>
            <xsl:text>&#10;</xsl:text>
        </xsl:for-each>


        <xsl:apply-templates select="protocol/vehicle/communications/ecus"/> 

        </body>
        </html>

    </xsl:template>


    <xsl:template match="protocol/information">
        <table>
            <tbody>
            <tr >
                <!-- name() get the name of  tag. -->    
                <td ><p>workshop_code:</p></td>
                <td ><p>version:</p></td>
            </tr>

            <tr>
                <td ><p><xsl:value-of select="workshop_code/serial_number"/></p></td>
                <!-- call template with more than one param. -->    
                <xsl:call-template name="formatText">
                    <!-- call template with static param -->    
                    <xsl:with-param name="tagName">Software Name</xsl:with-param>
                    <!-- call template with dynamic param --> 
                    <xsl:with-param name="inputText" select="software_name"/>
                </xsl:call-template>

            </tr>

            </tbody>
        </table>
    </xsl:template>

    <xsl:template match="information/workshop_code">
        <xsl:for-each select="child::*">
        <table>
            <tbody>
            <tr >
                <!-- name() get the name of  tag. -->    
                <xsl:variable name="tagName" select="name()"/>
                <xsl:if test="$tagName = 'serial_number'">
                    <td><b><xsl:value-of select="name()"/></b></td>
                </xsl:if>
                <td ><b><xsl:value-of select="."/></b></td>
            </tr>
            </tbody>
        </table>
        </xsl:for-each>         

    </xsl:template>

    <!-- format key value -->
    <xsl:template name="formatText">
        <xsl:param name="tagName"/>
        <xsl:param name="inputText"/>
        <td >
        <table>
            <tbody>
            <tr bgcolor="#9acd32">
                <!-- name() get the name of  tag. -->    
                <td ><p><xsl:value-of select="$tagName"/></p></td>
                <td ><p><xsl:value-of select="$inputText"/></p></td>
            </tr>
            </tbody>
        </table>
        </td>
    </xsl:template>


    <xsl:template match="information/version">
        <xsl:for-each select="child::*">
        <table>
            <tbody>
            <tr >
                <!-- name() get the name of  tag. -->    
                <td ><b><xsl:value-of select="name()"/></b></td>
                <td ><b><xsl:value-of select="."/></b></td>
            </tr>
            </tbody>
        </table>
        </xsl:for-each>         

    </xsl:template>    


    <xsl:template match="protocol/vehicle/communications/ecus">

        <table>
            <tbody>
            <tr >
                <!-- name() get the name of  tag. -->    
                <td>
                <p>Communications Data </p>
                <p margin-top="10pt" margin-bottom="10pt"> </p>
                
                <!-- child::name , Represent child of current node with name -->
                <xsl:for-each select="child::ecu">

                <!-- Generate id for each ecu id -->
                <xsl:variable name="ecu_id_tag" select="generate-id(child::ecu_id)"/>
                
                
                <input type="hidden" class="treeView" name="treeView_parse_start"  id="{concat('start_marker_treeView_master_entry_',$ecu_id_tag)}" />

                <table>
                    <tbody>
                    <tr>
                        <!-- name() get the name of  tag. --> 
                        <td>   
                            <p class="default_style_bu_diagfunction">
                            <span class="treeView_header">
                            <input type="checkbox" name="treeView_checkbox" onChange="javascript:showHideSectionsAfterCheckbox(this)" onClick="javascript:showHideSectionsAfterCheckbox(this)" class="treeView" id="{concat('treeView_master_entry_',$ecu_id_tag)}"/>
                            <label for="{concat('treeView_master_entry_',$ecu_id_tag)}" id="{concat('label_master_entry_',$ecu_id_tag)}" class="treeView">
                            <span class="treeView_selector_item">+</span>
                            <span class="treeView_label_content"><a name="{$ecu_id_tag}">Master - </a>001 Identification</span>
                            </label>
                            </span>
                            </p>
                        </td>
                        <td class="align_opposite" valign="top" style="width=65mm;">
                            <p class="default_style_diagfunction">28.02.2025 19:25:29 <a href="#inhalt">Agenda item^</a><br/></p>
                        </td>
                    </tr>
                    </tbody>
                </table>

                <span class="treeView_content" id="{concat('content_',$ecu_id_tag)}">
                <table class="default_style">
                    <xsl:for-each select="current()/ecu_master/values">
                    <tr>
                    <td valign="top" class="default_style" style="width:105mm">
                    <span class="default_style_b"></span>
                    <span class="default_style_b"><xsl:value-of select="display_name"/></span>
                    </td>
                    <td valign="top" colspan="2" style="width:95mm" class="default_style"><xsl:value-of select="display_value"/> </td>
                    </tr>
                    </xsl:for-each>
                </table>
                </span>


                </xsl:for-each>    
                <p></p>
                </td>
            </tr>
            </tbody>
        </table>

    </xsl:template>
    
</xsl:stylesheet>


