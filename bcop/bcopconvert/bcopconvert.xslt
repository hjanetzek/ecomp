<!--
  Ecomp option code generator

  Copyright : (C) 2007 by Dennis Kasprzyk
  E-mail    : onestone@beryl-project.org
 
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
-->
<xsl:stylesheet version='1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' >
    <xsl:output method="xml" indent="yes"/>
    
    <xsl:template  match="/plugin">
        <ecomp>
            <plugin>
                <xsl:attribute name="name">
                    <xsl:value-of select="@name"/>
                </xsl:attribute>
                <xsl:attribute name="useBcop">
                    <xsl:text>true</xsl:text>
                </xsl:attribute>
                <xsl:if test="display//option">
                    <display>
                        <xsl:apply-templates select="display/*"/>
                    </display>
                </xsl:if>
                <xsl:if test="screen//option">
                    <screen>
                        <xsl:apply-templates select="screen/*"/>
                    </screen>
                </xsl:if>
            </plugin>
        </ecomp>
    </xsl:template>

    <xsl:template match="group">
        <group>
	    <short>
		<xsl:value-of select="@name"/>
	    </short>
            <xsl:apply-templates select="./*"/>
        </group>
    </xsl:template>

    <xsl:template match="subgroup">
        <subgroup>
	    <short>
		<xsl:value-of select="@name"/>
	    </short>
            <xsl:apply-templates select="./*"/>
        </subgroup>
    </xsl:template>
    
    <xsl:template match="option">
        <option>
            <xsl:attribute name="name">
                <xsl:value-of select="@name"/>
            </xsl:attribute>
            <xsl:attribute name="type">
            <xsl:choose>
                <xsl:when test="@type = 'enum'">
                    <xsl:text>string</xsl:text>
                </xsl:when>
                <xsl:when test="@type = 'stringlist'">
                    <xsl:text>list</xsl:text>
                </xsl:when>
                <xsl:when test="@type = 'selection'">
                    <xsl:text>list</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="@type"/>
                </xsl:otherwise>
            </xsl:choose>
            </xsl:attribute>
            <short>
                <xsl:value-of select="./short/text()"/>
            </short>
            <long>
                <xsl:value-of select="./long/text()"/>
            </long>
            <xsl:if test="./hints">
                <hints>
                    <xsl:value-of select="./hints/text()"/>
                </hints>
            </xsl:if>
            <xsl:choose>
                <xsl:when test="@type = 'bool' and ./default">
                    <default>
                        <xsl:value-of select="./default/text()"/>
                    </default>
                </xsl:when>
                <xsl:when test="@type = 'int'">
                    <xsl:if test="./default">
			<default>
			    <xsl:value-of select="./default/text()"/>
			</default>
                    </xsl:if>
                    <xsl:if test="./min">
			<min>
			    <xsl:value-of select="./min/text()"/>
			</min>
                    </xsl:if>
                    <xsl:if test="./max">
			<max>
			    <xsl:value-of select="./max/text()"/>
			</max>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="@type = 'float'">
                    <xsl:if test="./default">
			<default>
			    <xsl:value-of select="./default/text()"/>
			</default>
                    </xsl:if>
                    <xsl:if test="./min">
			<min>
			    <xsl:value-of select="./min/text()"/>
			</min>
                    </xsl:if>
                    <xsl:if test="./max">
			<max>
			    <xsl:value-of select="./max/text()"/>
			</max>
                    </xsl:if>
                    <xsl:if test="./precision">
			<precision>
			    <xsl:value-of select="./precision/text()"/>
			</precision>
                    </xsl:if>
                </xsl:when>
                <xsl:when test="@type = 'string' and ./default">
                    <default>
                        <xsl:value-of select="./default/text()"/>
                    </default>
                </xsl:when>
                <xsl:when test="@type = 'match' and ./default">
                    <default>
                        <xsl:value-of select="./default/text()"/>
                    </default>
                </xsl:when>
                <xsl:when test="@type = 'color'">
                    <default>
                        <red><xsl:value-of select="./red/text()"/></red>
                        <green><xsl:value-of select="./green/text()"/></green>
                        <blue><xsl:value-of select="./blue/text()"/></blue>
                        <alpha><xsl:value-of select="./alpha/text()"/></alpha>
                    </default>
                </xsl:when>
                <xsl:when test="@type = 'enum'">
                    <default>
                        <xsl:value-of select="./value[@default = 'true'][1]"/>
                    </default>
                    <allowed>
                        <xsl:for-each select="./value">
                            <value><xsl:value-of select="text()"/></value>
                        </xsl:for-each>
                    </allowed>
                </xsl:when>
                <xsl:when test="@type = 'stringlist'">
                    <type>string</type>
                    <xsl:if test="./default">
			<default>
			    <xsl:for-each select="./default">
				<value><xsl:value-of select="text()"/></value>
			    </xsl:for-each>
			</default>
	            </xsl:if>
                </xsl:when>
                <xsl:when test="@type = 'selection'">
                    <type>string</type>
                    <default>
                        <xsl:for-each select="./value[@default = 'true']">
                            <value><xsl:value-of select="text()"/></value>
                        </xsl:for-each>
                    </default>
                    <allowed>
                        <xsl:for-each select="./value">
                            <value><xsl:value-of select="text()"/></value>
                        </xsl:for-each>
                    </allowed>
                </xsl:when>
                <xsl:when test="@type = 'action'">
                    <allowed>
                        <xsl:choose>
                            <xsl:when test="./key[contains(@state,'init') or not(@state)]">
                                <xsl:attribute name="key">
				    <xsl:text>true</xsl:text>
				</xsl:attribute>
                            </xsl:when>
                            <xsl:when test="./mouse[contains(@state,'init') or not(@state)]">
                                <xsl:attribute name="button">
				    <xsl:text>true</xsl:text>
				</xsl:attribute>
                            </xsl:when>
                            <xsl:when test="./edge[contains(@state,'init') or not(@state)]">
                                <xsl:attribute name="edge">
				    <xsl:text>true</xsl:text>
				</xsl:attribute>
                            </xsl:when>
                            <xsl:when test="./edge[contains(@state,'initdnd')]">
                                <xsl:attribute name="edgednd">
				    <xsl:text>true</xsl:text>
				</xsl:attribute>
                            </xsl:when>
                            <xsl:when test="./bell[contains(@state,'init') or not(@state)]">
                                <xsl:attribute name="bell">
				    <xsl:text>true</xsl:text>
				</xsl:attribute>
                            </xsl:when>
                        </xsl:choose>
                    </allowed>
                    <default>
                        <xsl:if test="./key">
                            <key>
                                <xsl:call-template name="convertBinding">
                                    <xsl:with-param name="binding">
			                 <xsl:value-of select="./key/text()"/>
		                    </xsl:with-param>
                                </xsl:call-template>
                            </key>
                        </xsl:if>
                        <xsl:if test="./mouse">
                            <button>
                                <xsl:call-template name="convertBinding">
                                    <xsl:with-param name="binding">
			                 <xsl:value-of select="./button/text()"/>
		                    </xsl:with-param>
                                </xsl:call-template>
                            </button>
                        </xsl:if>
                        <xsl:if test="./edge">
			    <edges>
			        <xsl:if test="contains(./edge/text(),'left')">
				    <xsl:attribute name="left">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'right')">
				    <xsl:attribute name="right">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'top')">
				    <xsl:attribute name="top">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'bottom')">
				    <xsl:attribute name="bottom">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'topleft')">
				    <xsl:attribute name="top_left">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'topright')">
				    <xsl:attribute name="top_right">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'bottomleft')">
				    <xsl:attribute name="bottom_left">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                                <xsl:if test="contains(./edge/text(),'bottomright')">
				    <xsl:attribute name="bottom_right">
					<xsl:text>true</xsl:text>
				    </xsl:attribute>
                                </xsl:if>
                            </edges>
	                </xsl:if>
                        <xsl:if test="./bell">
			    <bell><xsl:value-of select="./bell/text()"/></bell>
	                </xsl:if>
                    </default>
                </xsl:when>
            </xsl:choose>
        </option>
    </xsl:template>


    <xsl:template name="convertBinding">
        <xsl:param name="binding"/>
        <xsl:if test="string-length($binding)">
	    <xsl:if test="contains($binding,'+')">
		<xsl:call-template name="convertBinding">
		    <xsl:with-param name="binding">
			<xsl:value-of select="substring-before($binding,'+')"/>
		    </xsl:with-param>
		</xsl:call-template>
		<xsl:call-template name="convertBinding">
		    <xsl:with-param name="binding">
			<xsl:value-of select="substring-after($binding,'+')"/>
		    </xsl:with-param>
		</xsl:call-template>
	    </xsl:if>
	    <xsl:if test="not(contains($binding,'+'))">
                <xsl:choose>
                    <xsl:when test="contains('[Shift][Control][Mod1][Mod2][Mod3][Mod4][Mod5][Alt][Meta][Super][Hyper][ModeSwitch]',concat('[',concat($binding,']')))">
                        <xsl:text>&lt;</xsl:text>
                        <xsl:value-of select="$binding"/>
                        <xsl:text>&gt;</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:value-of select="$binding"/>
                    </xsl:otherwise>
                </xsl:choose>
	    </xsl:if>
	</xsl:if>
    </xsl:template>

</xsl:stylesheet>
