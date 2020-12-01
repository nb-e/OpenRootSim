<?xml version="1.0" encoding="utf-8"?>
<!--
Pretty XML Tree Viewer 1.0 (15 Oct 2001):
An XPath/XSLT visualisation tool for XML documents

Written by Mike J. Brown and Jeni Tennison.
No license; use freely, but please credit the authors if republishing elsewhere.

Adapated by jouke
Tree view of the simulation model. This stylesheet is adapted (reduced version) from the:
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="html" indent="no"/>
  <xsl:strip-space elements="*"/>
  <xsl:variable name="apos">'</xsl:variable>

<!--create page-->  
  <xsl:template match="/">
    <html>
      <head>
<meta http-equiv="content-type" content="text/html; charset=UTF-8" />
        <title>SimRoot Parametrization</title>
        <link type="text/css" rel="stylesheet" href="/InputFiles/XML/outlineviewBW.css"/>
      </head>
      <body>
        <h3><i>SimRoot</i> Parametrization</h3>
        <span class="description"><i>SimRoot</i> uses a hierarchical input file which is summarized below. The hierarchy gives the parameters context. 
        For example, the parameter 'specific leaf area' belongs to the shoot of a specific plant. In <i>SimRoot</i>, parameters can be a single value, 
        a value drawn from a distribution (random), or the result of an interpolation table. For constants we give the value, for distributions 
        the distribution parameters and for the tables a list of space separated values e.g. x1 y1 x2 y2 .... xn yn.<br/></span><br/>  
        
<!--insert the tree here by applying below templates to all child elements of current node (select=".")-->
        <ol class="tree">
           <xsl:for-each select="/SimulationModel/SimulaConstant[1]/*">
             <xsl:call-template name="main">
                 <xsl:with-param name="count" select="0"/>
             </xsl:call-template>
           </xsl:for-each>
        </ol>   
      </body>
    </html>
  </xsl:template>

<!--for all elements-->
<xsl:template match="*" mode="render" name="main">
   <xsl:param name="count" />

   <li class="level{$count}">
<!--write the tag's (elements) name-->
   <xsl:call-template name="writeElementNames"/>

<!--write data type -->
    <!--xsl:call-template name="writeDataTypes"/-->

<!--write name -->
    <xsl:call-template name="writeNames"/>
    
     <xsl:choose>
       <xsl:when test="local-name()='SimulaConstant'">
					<!--write values-->    
				   <xsl:call-template name="writeValues"/>
					<!--write units -->    
				   <xsl:call-template name="writeUnits"/>
       </xsl:when>
       <xsl:otherwise>
					<!--write units -->    
				   <xsl:call-template name="writeUnits"/>
					<!--write names $ units of table columns 1-->    
				   <xsl:call-template name="dependence"/>
					<!--write values-->    
				   <xsl:call-template name="writeValues"/>
      </xsl:otherwise>
     </xsl:choose>
 
<!--hide all other attributes by writing nothing-->
    <xsl:apply-templates select="@*" mode="render"/>
<!--do a this recursively for all children-->
    <xsl:if test="*">
	    <ol>
	    <xsl:for-each select="*">
	        <xsl:call-template name="main">
	        	<xsl:with-param name="count" select="$count + 1"/>
	        </xsl:call-template>
	    </xsl:for-each>
	    </ol>
    </xsl:if>
    </li>
</xsl:template>

<!--template for writing the element names SimulaBase, etc-->
<xsl:template name="writeElementNames">
     <span class="element"> 
     <xsl:choose>
       <xsl:when test="local-name()='SimulationModelIncludeFile'">
			    <xsl:text>Origin</xsl:text>
       </xsl:when>
       <xsl:when test="local-name()='SimulationModel'">
			    <xsl:text>Origin</xsl:text>
       </xsl:when>
       <xsl:otherwise>
		    <!--xsl:value-of select="local-name()"/-->
      </xsl:otherwise>
     </xsl:choose>
     </span>
</xsl:template>

<!--template for writing the simula names-->
<xsl:template name="writeDataTypes">
     <span class="type">
     <xsl:choose>
       <xsl:when test="@type">
         <xsl:text> [</xsl:text><xsl:value-of select="@type"/><xsl:text>]</xsl:text>
       </xsl:when>
       <xsl:when test="local-name()='SimulaBase'">
           <xsl:text> [container]</xsl:text>
       </xsl:when>
       <xsl:when test="local-name()='SimulationModel'">
           <xsl:text></xsl:text>
       </xsl:when>
       <xsl:when test="local-name()='SimulationModelIncludeFile'">
           <xsl:text></xsl:text>
       </xsl:when>
       <xsl:otherwise>
           <xsl:text> [float]</xsl:text>
      </xsl:otherwise>
     </xsl:choose>
     </span>
</xsl:template>

<!--template for writing the simula names-->
<xsl:template name="writeNames">
     <xsl:choose>
       <xsl:when test="@prettyName">
    		 <xsl:if test="*[1]"><!--todo would better if we would do this based on wether it as children-->
          <span class="basename"><xsl:text> '</xsl:text><xsl:value-of select="@prettyName"/><xsl:text>'</xsl:text></span>
         </xsl:if>
    		 <xsl:if test="not(*[1])">
          <span class="name"><xsl:text> '</xsl:text><xsl:value-of select="@prettyName"/><xsl:text>'</xsl:text></span>
         </xsl:if>
       </xsl:when>
       <xsl:when test="@name">
    		 <xsl:if test="local-name()='SimulaBase'">
          <span class="basename"><xsl:text> '</xsl:text><xsl:value-of select="@name"/><xsl:text>'</xsl:text></span>
         </xsl:if>
    		 <xsl:if test="local-name()!='SimulaBase'">
          <span class="name"><xsl:text> '</xsl:text><xsl:value-of select="@name"/><xsl:text>'</xsl:text></span>
         </xsl:if>
       </xsl:when>
       <xsl:when test="@name_colum2">
		     <span class="name"><xsl:text> '</xsl:text><xsl:value-of select="@name_colum2"/><xsl:text>'</xsl:text></span>
       </xsl:when>
       <xsl:when test="@path">
		     <span class="xpath"><xsl:text> '</xsl:text><xsl:value-of select="@path"/><xsl:text>'</xsl:text></span>
       </xsl:when>
       <xsl:when test="@fileName">
		     <span class="extref"><xsl:text> '</xsl:text><xsl:value-of select="@fileName"/><xsl:text>'</xsl:text></span>
       </xsl:when>
       <xsl:otherwise>
      </xsl:otherwise>
     </xsl:choose>
</xsl:template>

<!--template for writing the simula units-->
<xsl:template name="writeUnits">
    <xsl:choose>
       <xsl:when test="@unit">
			   <span class="unit">
			      <xsl:text>(</xsl:text>
			      
			      
			      
			      
			      
			      
			      
			      <xsl:call-template name="insertSuperscripts">
						<xsl:with-param name="nameString" select="@unit"/>
					</xsl:call-template>			      
			      
			      <!--xsl:value-of select="@unit"/-->
			      <xsl:text>)</xsl:text>
			   </span>
       </xsl:when>
       <xsl:when test="@unit_colum2">
			   <span class="unit"><xsl:text> (</xsl:text>

			      <xsl:call-template name="insertSuperscripts">
						<xsl:with-param name="nameString" select="@unit_colum2"/>
					</xsl:call-template>			      

			   <!--xsl:value-of select="@unit_colum2"/-->
			   <xsl:text>)</xsl:text></span>
       </xsl:when>
       <xsl:otherwise>
      </xsl:otherwise>
     </xsl:choose>
</xsl:template>

<!--template for super script in units-->
<xsl:template name="insertSuperscripts">	
					<xsl:param name="nameString" select="string()"/>		      
		      	<xsl:variable name="firstChar" select="substring($nameString,1,1)"/>
						<xsl:choose>
					    <xsl:when test="contains($firstChar,'^')">
					      <sup>
					        <xsl:value-of select="substring-before(substring($nameString,2),'*')"/>
					      </sup>
			      <xsl:call-template name="insertSuperscripts">
								      <xsl:with-param name="nameString" select="substring-after($nameString,'*')"/>
					</xsl:call-template>			      
					    </xsl:when>
					    <xsl:otherwise>
					      <xsl:choose>
					         <xsl:when test="contains($nameString,'^')">
					            <xsl:value-of select="substring-before($nameString,'^')"/>
			      <xsl:call-template name="insertSuperscripts">
								      <xsl:with-param name="nameString" select="concat('^',substring-after($nameString,'^'))"/>
					</xsl:call-template>			      
					         </xsl:when>
					    		<xsl:otherwise>  
					    		  <xsl:value-of select="$nameString"/>
					         </xsl:otherwise>
					  </xsl:choose>
			    </xsl:otherwise>
			  </xsl:choose>
</xsl:template>								      


<xsl:template name="dependence">
     <xsl:choose>
       <xsl:when test="@name_colum1">
	       <span class="name"><xsl:text>=f{'</xsl:text><xsl:value-of select="@name_colum1"/><xsl:text>'}</xsl:text></span>
       </xsl:when>
       <xsl:when test="@distribution">
	       <span class="name"><xsl:text>=f{'</xsl:text><xsl:value-of select="@distribution"/><xsl:text> distribution'}</xsl:text></span>
       </xsl:when>
       <xsl:otherwise>
       </xsl:otherwise>
     </xsl:choose>
     <xsl:choose>
       <xsl:when test="@unit_colum1">
	       <span class="unit"><xsl:text> (</xsl:text><xsl:value-of select="@unit_colum1"/><xsl:text>)</xsl:text></span>
       </xsl:when>
       <xsl:otherwise>
      </xsl:otherwise>
     </xsl:choose>
</xsl:template>


<xsl:template name="writeValues" mode="render">
     <span class="values">
       <xsl:if test="@minimum">
         <xsl:text> minimum=</xsl:text><xsl:value-of select="@minimum"/><xsl:text></xsl:text>
       </xsl:if>
       <xsl:if test="@maximum">
         <xsl:text> maximum=</xsl:text><xsl:value-of select="@maximum"/><xsl:text></xsl:text>
       </xsl:if>
       <xsl:if test="@mean">
         <xsl:text> mean=</xsl:text><xsl:value-of select="@mean"/><xsl:text></xsl:text>
       </xsl:if>
       <xsl:if test="@stdev">
         <xsl:text> stdev=</xsl:text><xsl:value-of select="@stdev"/><xsl:text></xsl:text>
       </xsl:if>
       <xsl:if test="text()">

     <xsl:choose>
         <xsl:when test="local-name()='SimulaConstant'">
     	  		<xsl:text> = </xsl:text><xsl:value-of select="text()"/><xsl:text></xsl:text>
         </xsl:when>
         <xsl:when test="local-name()='SimulaTable'">
      	 		<!--br/--><xsl:text> x,y pairs :{ </xsl:text><xsl:value-of select="text()"/><xsl:text>}</xsl:text>
         </xsl:when>
         <xsl:when test="local-name()='SimulaVariable'">
       		<xsl:text> initial value = </xsl:text><xsl:value-of select="text()"/><xsl:text></xsl:text>
         </xsl:when>
         <xsl:when test="local-name()='SimulaPoint'">
       		<xsl:text> initial position = </xsl:text><xsl:value-of select="text()"/><xsl:text></xsl:text>
         </xsl:when>
         <xsl:otherwise>
         </xsl:otherwise>
     </xsl:choose>
       </xsl:if>
     </span>
</xsl:template>


<!--hide all other attributes -->
<xsl:template match="@*" mode="render">
</xsl:template>

<!--write the ascii art hierarchy lines something like "space |____"   -->
  <xsl:template name="ascii-art-hierarchy">
    <!--we want to insert vertical bars for all the parents -->  
    <xsl:for-each select="ancestor::*">
      <!--if this is the top of the tree, or not-->
      <xsl:choose>
        <xsl:when test="following-sibling::node()">
          <!--its not the top of the tree, insert vertical bars with spaces-->      
          <span class='connector'>&#160;&#160;</span>|<span class='connector'>&#160;&#160;</span>
          <xsl:text>&#160;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <!--this is the top of the tree, insert initial spacing-->
          <span class='connector'>&#160;&#160;&#160;&#160;</span>
          <span class='connector'>&#160;&#160;</span>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:choose>
      <!-- print tagname -->
      <xsl:when test="parent::node() and ../child::node()">
        <!--print child node-->
        <!--insert two non breaking spaces-->
        <span class='connector'>&#160;&#160;</span>
        <!--insert vertical bar-->
        <xsl:text>|</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <span class='connector'>&#160;&#160;&#160;</span>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

<!-- recursive template to escape linefeeds, tabs -->
  <xsl:template name="escape-ws">
    <xsl:param name="text"/>
    <xsl:choose>
      <xsl:when test="contains($text, '&#xA;')">
        <xsl:call-template name="escape-ws">
          <xsl:with-param name="text" select="substring-before($text, '&#xA;')"/>
        </xsl:call-template>
        <span class="escape">\n</span>
        <xsl:call-template name="escape-ws">
          <xsl:with-param name="text" select="substring-after($text, '&#xA;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="contains($text, '&#x9;')">
        <xsl:value-of select="substring-before($text, '&#x9;')"/>
        <span class="escape">\t</span>
        <xsl:call-template name="escape-ws">
          <xsl:with-param name="text" select="substring-after($text, '&#x9;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$text"/></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  
 <!--function to replace strings--> 
 <xsl:template name="string-replace-all">
    <xsl:param name="text" />
    <xsl:param name="replace" />
    <xsl:param name="by" />
    <xsl:choose>
      <xsl:when test="contains($text, $replace)">
        <xsl:value-of select="substring-before($text,$replace)" />
        <xsl:value-of select="$by" />
        <xsl:call-template name="string-replace-all">
          <xsl:with-param name="text"
          select="substring-after($text,$replace)" />
          <xsl:with-param name="replace" select="$replace" />
          <xsl:with-param name="by" select="$by" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>  

</xsl:stylesheet>
