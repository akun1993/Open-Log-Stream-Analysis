<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="html" indent="yes"/>
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
    </head>


            <body>


                <h1>Library Books</h1>


                <table border="1">


                    <tr>


                        <th>Title</th>


                        <th>Author</th>


                        <th>Year</th>


                    </tr>


                    <xsl:for-each select="library/book">


                        <tr>


                            <td><xsl:value-of select="title"/></td>


                            <td><xsl:value-of select="author"/></td>


                            <td><xsl:value-of select="year"/></td>


                        </tr>


                    </xsl:for-each>


                </table>


            </body>


        </html>


    </xsl:template>


</xsl:stylesheet>

