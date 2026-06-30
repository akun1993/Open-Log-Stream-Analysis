
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.1">
    <xsl:output method="html" version="1.1" encoding="utf-8" indent="yes"/>

    <xsl:template match="/">

        <html>
    <head>
        <style type="text/css">
				html {direction:ltr;box-sizing:border-box}
				*,*::before,*::after {box-sizing:inherit}
				body {font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Arial,sans-serif;font-size:10pt;color:#333;margin:0;padding:16px;background:#f5f7fa}
				table {border-width:0pt;border-color:black;border-style:solid;border-collapse:collapse;padding:0;margin:0;width:100%;table-layout:auto}
				.border_style {border-width:1pt;border-color:black;border-style:solid;border-collapse:collapse;padding-left:5;padding-right:5;margin-top:10;margin-bottom:10;width:100%}
				tr {border-width:0pt;border-color:black;border-style:solid;padding:0;margin:0;text-align:left}
				td {border-width:0pt;border-color:black;border-style:solid;padding:2px 4px;margin:0;text-align:left}
			.align_opposite {text-align:right;}
			span {margin-top:0pt;margin-bottom:0pt}
				div {margin-top:0pt;margin-bottom:10pt}
			p {margin-top:0pt;margin-bottom:0pt}
			.p_mini {margin-top:0pt;margin-bottom:0pt}
			.p_small {margin-top:10pt;margin-bottom:10pt}
			.p_big {margin-top:16pt;margin-bottom:16pt}
			.courier_style {margin-top:0;margin-bottom:0;font-size:10pt;font-family:"Courier New";direction:ltr}
			.default_obd_mode {color:blue;margin-top:0;margin-bottom:0;font-size:10pt;font-weight:bold;font-family:Arial;direction:ltr}
			.default_error {color:#dc2626;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
			.default_error_b {color:#dc2626;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
			.default_warn {color:#d97706;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
			.default_warn_b {color:#d97706;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
			.default_ok {color:#16a34a;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
			.compstylenotequal {color:#dc2626;margin-top:0;margin-bottom:0;font-size:10pt;font-weight:bold;font-family:Arial;direction:ltr}
			.compstyleequal {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
			.default_style {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
			.default_style_revert_align {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;direction:right}
			.default_style_topmargin {margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;direction:ltr}
			.default_style_b {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
			.default_style_b_indent {text-indent:20pt;margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
			.default_style_bu {margin-top:0;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;text-decoration:underline;direction:ltr}
			.default_style_bu_topmargin {margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;text-decoration:underline;direction:ltr}
				.default_style_bu_diagfunction {background:linear-gradient(135deg,#2d6da3,#1a3a5c);color:#fff;margin-top:15;margin-bottom:0;font-size:11pt;font-family:Arial;font-weight:bold;direction:ltr;padding:8px 12px;border-radius:4px}
				.default_style_diagfunction {background:linear-gradient(135deg,#2d6da3,#1a3a5c);color:#fff;margin-top:15;margin-bottom:0;font-size:11pt;font-family:Arial;direction:ltr;padding:8px 12px;border-radius:4px}

				/* App section header bar */
				.app-header-bar {background:linear-gradient(135deg,#2d6da3,#1a3a5c);color:#fff;padding:10px 16px;border-radius:6px 6px 0 0;display:flex;align-items:center;justify-content:space-between;cursor:pointer;user-select:none}
				.app-header-bar:hover {filter:brightness(1.1)}
				.app-event-count {font-size:9pt;font-weight:400;opacity:0.85}
				.app-top-link {color:#fff;opacity:0.7;text-decoration:none;font-size:8pt;margin-left:12px;white-space:nowrap}
				.app-top-link:hover {opacity:1;text-decoration:underline}
			.default_style_ecu {background-color:#8FBCE6;margin-top:15;margin-bottom:0;font-size:10pt;font-family:Arial;font-weight:bold;direction:ltr}
			.default_style_big_bi {margin-top:0;margin-bottom:0;font-size:16pt;font-family:Arial;font-weight:bold;font-style:italic;direction:ltr}
			.default_style_big_b {margin-top:0;margin-bottom:0;font-size:20pt;font-family:Arial;font-weight:bold;direction:ltr}
			.header_style_big {margin-top:0;margin-bottom:0;font-size:14pt;font-family:Arial;font-weight:bold;direction:ltr;color:#fff}
			.header_style_bigu {margin-top:0;margin-bottom:0;font-size:13pt;font-family:Arial;font-weight:bold;direction:ltr;color:#1a3a5c;border-bottom:2px solid #2d6da3;padding-bottom:4px}
			.header_style_small {margin-top:0;margin-bottom:0;font-size:11pt;font-family:Arial;direction:ltr;color:#fff;opacity:0.8}
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
				table.small_table {border-width:0pt;border-color:black;border-style:solid;border-collapse:collapse;padding:0;margin:0;width:auto;table-layout:auto;font-size:10pt;font-family:Arial}

			/* Styles for the expansion mechanism */
			input.treeView {float:left !important; z-index:-1 !important; position:absolute; vertical-align:middle;  width:0px; height:0px; overflow:hidden;}
			label.treeView.hover {text-decoration:underline; vertical-align:middle;}
			label.treeView {cursor:pointer; vertical-align:middle; color:#fff;}
			.treeView_label_content { position:relative; left:1.5em; vertical-align:middle;}
			.treeView_selector_item { position:absolute; display:table-cell; vertical-align:middle; border:1px solid; padding-left:0.33em; padding-right:0.33em; background-color:#fff; font-family:Consolas,'Courier New',Courier; margin-top:0;margin-bottom:0;font-size:10pt;font-weight:bold;direction:ltr;color:#2d6da3}

				.treeView_content {clear:left; margin-left:0; width:100%; background-color:#ffffff; border-radius:0 0 6px 6px; box-shadow:0 1px 3px rgba(0,0,0,0.08); overflow:hidden; display:none; margin-bottom:20px}

				/* Report header card */
				.ols-header {background:linear-gradient(135deg,#1a3a5c 0%,#2d6da3 100%);padding:20px 30px;border-radius:8px;margin-bottom:16px;display:flex;align-items:flex-end;justify-content:space-between}
				.ols-header h1 {margin:0;font-size:18pt;font-weight:700;color:#fff;letter-spacing:0.5px}
				.ols-header h2 {margin:0;font-size:13pt;font-weight:400;color:#fff;opacity:0.9}
				.ols-header .timestamp {font-size:9pt;color:#fff;opacity:0.7;text-align:right}

			/* Content TOC */
			.toc-table {width:100%;border-collapse:collapse;margin:0;padding:0}
			.toc-table th {text-align:left;padding:8px 10px;background:#e8eef5;color:#1a3a5c;font-size:9pt;font-weight:600;border-bottom:2px solid #c5d4e6}
			.toc-table td {padding:6px 10px;border-bottom:1px solid #eef1f5;font-size:10pt}
			.toc-table tr:hover td {background:#f0f5fc}
			.toc-table a {color:#2d6da3;text-decoration:none;font-weight:500}
			.toc-table a:hover {text-decoration:underline}
				.toc-card {background:#fff;padding:20px 24px;border-radius:8px;box-shadow:0 1px 3px rgba(0,0,0,0.08);margin-bottom:16px}
				.toc-card h2 {margin:0 0 12px 0;font-size:13pt;color:#1a3a5c;border-bottom:2px solid #2d6da3;padding-bottom:6px}
				.count-badge {display:inline-block;background:#e8eef5;color:#2d6da3;border-radius:10px;padding:1px 8px;font-size:8pt;font-weight:600}

			/* System info */
				.system-card {background:#fff;padding:12px 20px;margin:12px 0;border-radius:6px;box-shadow:0 1px 3px rgba(0,0,0,0.08);display:flex;align-items:center;flex-wrap:wrap;gap:4px}
				.system-card .label {font-weight:600;color:#1a3a5c;margin-right:4px}

			/* Detail data table inside treeView_content */
			.detail-table {width:100%;border-collapse:collapse;font-size:9pt;font-family:Arial;direction:ltr}
			.detail-table th {text-align:left;padding:6px 10px;background:#f0f5fc;color:#1a3a5c;font-size:8pt;font-weight:600;border-bottom:2px solid #d4dfed}
			.detail-table td {padding:4px 10px;border-bottom:1px solid #eef1f5;vertical-align:top}
			.detail-table tr:nth-child(even) td {background:#fafbfd}
			.detail-table tr:hover td {background:#eef3fa}
				.col-time {white-space:nowrap;color:#666;min-width:80px}
				.col-title {font-weight:500;min-width:60px}
				.col-msg {font-family:Consolas,'Courier New',monospace;font-size:8.5pt;color:#444;word-break:break-word;overflow-wrap:break-word}
			.col-count {text-align:center;font-weight:600;color:#2d6da3;width:50px}

				@media screen
				{
				  body { margin-top:0;margin-bottom:0 }
				  .table_spec2 {border-width:0;border-collapse:collapse;padding:0;margin:0;width:100%}
				  p {margin-top:0pt;margin-bottom:0pt }
				  .nichtdrucken {font-size:10pt;font-family:Arial;direction:ltr}
				}

				@media screen and (max-width:768px)
				{
				  body {padding:8px}
				  .ols-header {padding:12px 16px;flex-direction:column;align-items:flex-start}
				  .ols-header .timestamp {text-align:left;margin-top:8px}
				  .ols-header h1 {font-size:14pt}
				  .ols-header h2 {font-size:11pt}
				  .toc-card, .system-card {padding:12px;border-radius:6px}
				  .detail-table {font-size:8pt}
				  .detail-table th, .detail-table td {padding:3px 6px}
				  .col-time {font-size:7pt;white-space:normal}
				  .col-msg {font-size:7.5pt}
				  .default_style_bu_diagfunction, .default_style_diagfunction {font-size:10pt;padding:6px 8px}
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

    <title>OLS Analysis Report</title>
    </head>

        <body>

						<div class="ols-header">
							<div>
								<h1>OLS Analysis Engine</h1>
								<h2>Analysis Report</h2>
							</div>
							<div class="timestamp">
								<xsl:value-of select="protocol/system/timestamp"/>
							</div>
						</div>

	        <xsl:apply-templates select="protocol/system"/>

						<!-- TOC card -->
						<div class="toc-card">
							<h2><a name="inhalt">Content</a></h2>
							<table class="toc-table">
								<thead>
								<tr>
									<th>App Name</th>
									<th>Total Events</th>
								</tr>
								</thead>
								<tbody>
								<xsl:for-each select="descendant::app">
										<tr>
									  <td><a href="#{generate-id(name)}"><xsl:value-of select="name"/></a></td>
									  <td><span class="count-badge"><xsl:value-of select="count(analysis/item)"/></span></td>
										</tr>
								</xsl:for-each>
								</tbody>
							</table>
						</div>

					<p class="p_small"></p>

					<table>
						<tbody>

        	<xsl:apply-templates select="protocol/applications/apps"/>

						</tbody>
					</table>

        </body>
        </html>

    </xsl:template>


    <xsl:template match="protocol/system">
					<div class="system-card">
						<span class="label">Vehicle</span>&#160;&#160;
						<xsl:call-template name="formatText">
							<xsl:with-param name="tagName">Software Name</xsl:with-param>
							<xsl:with-param name="inputText" select="software_name"/>
						</xsl:call-template>
					</div>
    </xsl:template>

    <xsl:template match="information/workshop_code">
        <xsl:for-each select="child::*">
        <table>
            <tbody>
            <tr >
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

        <table style="display:inline;width:auto;margin:0;">
            <tbody>
            <tr>
								<td class="default_style"></td>
								<td class="default_style"><b><xsl:value-of select="$tagName"/></b></td>
								<td class="default_style"><b>: </b></td>
								<td class="default_style">&#160;</td>
								<td class="default_style"><xsl:value-of select="$inputText"/></td>
            </tr>
            </tbody>
        </table>
    </xsl:template>


    <xsl:template match="information/version">
        <xsl:for-each select="child::*">
        <table>
            <tbody>
            <tr >
                <td ><b><xsl:value-of select="name()"/></b></td>
                <td ><b><xsl:value-of select="."/></b></td>
            </tr>
            </tbody>
        </table>
        </xsl:for-each>
    </xsl:template>


    <xsl:template match="protocol/applications/apps">

				<tr >
						<td>
						<p class="header_style_bigu">Applications analysis result</p>
						<p class="p_small"></p>

						<!-- child::name , Represent child of current node with name -->
								<xsl:for-each select="child::app">
								<!-- Generate id for each ecu id -->
								<xsl:variable name="app_id_tag" select="generate-id(child::name)"/>

									<input type="hidden" class="treeView" name="treeView_parse_start"  id="{concat('start_marker_treeView_master_entry_',$app_id_tag)}" />

									<div class="app-header-bar">
										<xsl:if test="position() > 1">
											<xsl:attribute name="style">margin-top:20px</xsl:attribute>
										</xsl:if>
														<input type="checkbox" name="treeView_checkbox" onChange="javascript:showHideSectionsAfterCheckbox(this)" onClick="javascript:showHideSectionsAfterCheckbox(this)" class="treeView" id="{concat('treeView_master_entry_',$app_id_tag)}"/>
														<label for="{concat('treeView_master_entry_',$app_id_tag)}" id="{concat('label_master_entry_',$app_id_tag)}" class="treeView">
														<span class="treeView_selector_item">+</span>
														<span class="treeView_label_content"><a name="{$app_id_tag}" style="color:inherit;text-decoration:none"><xsl:value-of select="child::name"/></a>&#160;—&#160;Analysis Result&#160;<span class="app-event-count">(<xsl:value-of select="count(analysis/item)"/> events)</span></span>
														</label>
														<a href="#inhalt" class="app-top-link" onclick="event.stopPropagation();">Top^</a>
								</div>

							<span class="treeView_content" id="{concat('content_',$app_id_tag)}">
								<table class="detail-table">
										<thead>
										<tr>
											<th class="col-time">Time</th>
											<th class="col-title">Title</th>
											<th class="col-msg">Message</th>
										</tr>
										</thead>
										<tbody>
										<xsl:for-each select="analysis/item">
										<tr>
										<td class="col-time"><xsl:value-of select="time"/></td>
										<td class="col-title"><xsl:value-of select="title"/></td>
										<td class="col-msg"><xsl:value-of select="message"/></td>
										</tr>
										</xsl:for-each>
										</tbody>
								</table>
								</span>

							</xsl:for-each>
							</td>
				</tr>

    </xsl:template>

</xsl:stylesheet>
