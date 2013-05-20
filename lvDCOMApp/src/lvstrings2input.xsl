<?xml version="1.0" encoding="UTF-8"?>
<!--
    ########### SVN repository information ###################
    # $LastChangedDate$
    # $LastChangedBy$
    # $LastChangedRevision$
    # $HeadURL$
    ########### SVN repository information ###################

    Usage: xsltproc lvstrings2input.xsl lvexport.xml > lvinput.xml

    lvexport.xml is the output of ExportVIStrings() from LabVIEW, but has also 
	             been processed by the fix_xml.cmd  script	(as the raw output is not true XML)

	@author Freddie Akeroyd, STFC ISIS Facility, UK
-->
<xsl:stylesheet
    version="1.0"
    xmlns:xs="http://www.w3.org/2001/XMLSchema"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:lvdcom="http://epics.isis.rl.ac.uk/lvDCOMinput/1.0">
    
    <xsl:output method="xml" indent="yes" version="1.0" encoding="UTF-8"/>
    
    <xsl:template match="/VI">
        <xsl:comment>Generated by $Id$</xsl:comment>
        <xsl:variable name="vi_name" select="@name" />
		<xsl:element name="lvinput">
		    <xsl:attribute name="xmlns">http://epics.isis.rl.ac.uk/lvDCOMinput/1.0</xsl:attribute>
		    <xsl:attribute name="xsi:schemaLocation">http://epics.isis.rl.ac.uk/lvDCOMinput/1.0 lvDCOMinput.xsd</xsl:attribute>
		    <xsl:element name="extint">
		       <xsl:attribute name="path">c:/LabVIEW Modules/Common/External Interface/External Interface.llb/External Interface - Set Value.vi</xsl:attribute> 
			</xsl:element>
		<xsl:element name="section" >
		       <xsl:attribute name="name">example</xsl:attribute> 
		    <xsl:element name="vi">
		        <xsl:attribute name="path"><xsl:value-of select="$vi_name"/></xsl:attribute> 
		        <xsl:apply-templates select="CONTENT" />
		    </xsl:element>
		</xsl:element>
		    </xsl:element>
	</xsl:template>
	
   <xsl:template match="CONTENT">
           <xsl:apply-templates select="CONTROL" />
	</xsl:template>

   <xsl:template match="CONTROL">
        <xsl:variable name="control_type" select="@type" />
        <xsl:variable name="control_id" select="@ID" />
        <xsl:variable name="control_name" select="@name" />
		<xsl:element name="param">
		<xsl:attribute name="name"><xsl:value-of select="$control_name"/></xsl:attribute>  
		<xsl:attribute name="type"><xsl:value-of select="$control_type"/></xsl:attribute> 
		<xsl:element name="read">
		    <xsl:attribute name="method">GCV</xsl:attribute>  
		    <xsl:attribute name="target"><xsl:value-of select="$control_name"/></xsl:attribute> 
		</xsl:element>
		    <xsl:element name="set">
		        <xsl:attribute name="method">SCV</xsl:attribute>  
		        <xsl:attribute name="extint">false</xsl:attribute>  
		        <xsl:attribute name="target"><xsl:value-of select="$control_name"/></xsl:attribute> 
		    </xsl:element>
		</xsl:element>
	</xsl:template>
   
</xsl:stylesheet>

<!--
   /CONTENT/CONTROL/@type    Numeric(ID=80)  String(ID=81)  Array(ID=82) Boolean(ID=79)    Cluster    "Radio Buttons" "Ring" "Listbox" "Enum" "Type Definition"
	/CONTENT/CONTROL/@name
	
	if array, /CONTENT/CONTROL/CONTENT/CONTROL/@type  Numeric
-->