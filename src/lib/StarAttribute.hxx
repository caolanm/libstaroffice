/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstaroffice
* Version: MPL 2.0 / LGPLv2+
*
* The contents of this file are subject to the Mozilla Public License Version
* 2.0 (the "License"); you may not use this file except in compliance with
* the License or as specified alternatively below. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Major Contributor(s):
* Copyright (C) 2002 William Lachance (wrlach@gmail.com)
* Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
* Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
* Copyright (C) 2006, 2007 Andrew Ziem
* Copyright (C) 2011, 2012 Alonso Laurent (alonso@loria.fr)
*
*
* All Rights Reserved.
*
* For minor contributions see the git repository.
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
* in which case the provisions of the LGPLv2+ are applicable
* instead of those above.
*/

/*
 * file to read/parse StarOffice attributes
 *
 */
#ifndef STAR_ATTRIBUTE
#  define STAR_ATTRIBUTE

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

namespace StarAttributeInternal
{
struct State;
}

class StarZone;
class StarDocument;

/** \brief the main class to read/.. a StarOffice attribute
 *
 *
 *
 */
class StarAttribute
{
public:
  //! the attribute list
  enum {
    ATTR_CHR_CASEMAP = 1,					        	    		// 1
    ATTR_CHR_CHARSETCOLOR,					        	    	// 2
    ATTR_CHR_COLOR,								        	    		// 3
    ATTR_CHR_CONTOUR,							        	    		// 4
    ATTR_CHR_CROSSEDOUT,						        	    	// 5
    ATTR_CHR_ESCAPEMENT,						        	    	// 6
    ATTR_CHR_FONT,									        	    	// 7
    ATTR_CHR_FONTSIZE,							        	    	// 8
    ATTR_CHR_KERNING, 							        	    	// 9
    ATTR_CHR_LANGUAGE,							        	    	// 10
    ATTR_CHR_POSTURE, 							        	    	// 11
    ATTR_CHR_PROPORTIONALFONTSIZE,		      	    	// 12
    ATTR_CHR_SHADOWED,							        	    	// 13
    ATTR_CHR_UNDERLINE,						        	    		// 14
    ATTR_CHR_WEIGHT,								        	    	// 15
    ATTR_CHR_WORDLINEMODE,					        	    	// 16
    ATTR_CHR_AUTOKERN,							        	    	// 17
    ATTR_CHR_BLINK,								        	    		// 18
    ATTR_CHR_NOHYPHEN,							        	    	// 19
    ATTR_CHR_NOLINEBREAK, 					        	    	// 20
    ATTR_CHR_BACKGROUND,						        	    	// 21
    ATTR_CHR_CJK_FONT,							        	    	// 22
    ATTR_CHR_CJK_FONTSIZE,					        	    	// 23
    ATTR_CHR_CJK_LANGUAGE,					        	    	// 24
    ATTR_CHR_CJK_POSTURE, 					        	    	// 25
    ATTR_CHR_CJK_WEIGHT,						        	    	// 26
    ATTR_CHR_CTL_FONT,							        	    	// 27
    ATTR_CHR_CTL_FONTSIZE,					        	    	// 28
    ATTR_CHR_CTL_LANGUAGE,					        	    	// 29
    ATTR_CHR_CTL_POSTURE, 					        	    	// 30
    ATTR_CHR_CTL_WEIGHT,						        	    	// 31
    ATTR_CHR_ROTATE,								        	    	// 32
    ATTR_CHR_EMPHASIS_MARK,				        	    		// 33
    ATTR_CHR_TWO_LINES, 						        	    	// 34
    ATTR_CHR_SCALEW,								        	    	// 35
    ATTR_CHR_RELIEF,								        	    	// 36
    ATTR_CHR_DUMMY1,								        	    	// 37

    ATTR_TXT_INETFMT,							        	    		// 38
    ATTR_TXT_DUMMY4,								        	    	// 39
    ATTR_TXT_REFMARK, 							        	    	// 40
    ATTR_TXT_TOXMARK, 							        	    	// 41
    ATTR_TXT_CHARFMT, 							        	    	// 42
    ATTR_TXT_DUMMY5, 								      	    		// 43
    ATTR_TXT_CJK_RUBY, 							      	    		// 44
    ATTR_TXT_UNKNOWN_CONTAINER,					  	    		// 45
    ATTR_TXT_DUMMY6,								        	    	// 46
    ATTR_TXT_DUMMY7,								        	    	// 47

    ATTR_TXT_FIELD,								        	    		// 48
    ATTR_TXT_FLYCNT,								        	    	// 49
    ATTR_TXT_FTN, 									        	    	// 50
    ATTR_TXT_SOFTHYPH,							        	    	// 51
    ATTR_TXT_HARDBLANK,							      	    		// 52
    ATTR_TXT_DUMMY1,								        	    	// 53
    ATTR_TXT_DUMMY2,								        	    	// 54

    ATTR_PARA_LINESPACING,		              	    	// 55
    ATTR_PARA_ADJUST,								    	    			// 56
    ATTR_PARA_SPLIT,								              	// 57
    ATTR_PARA_ORPHANS, 							    	    			// 58
    ATTR_PARA_WIDOWS,								    	    			// 59
    ATTR_PARA_TABSTOP, 							    	    			// 60
    ATTR_PARA_HYPHENZONE,							    	    		// 61
    ATTR_PARA_DROP,								              		// 62
    ATTR_PARA_REGISTER,							    	    			// 63
    ATTR_PARA_NUMRULE, 							    	    			// 64
    ATTR_PARA_SCRIPTSPACE,							    	    	// 65
    ATTR_PARA_HANGINGPUNCTUATION,					    	    // 66
    ATTR_PARA_FORBIDDEN_RULES,						    	    // 67
    ATTR_PARA_VERTALIGN,								            // 68
    ATTR_PARA_SNAPTOGRID,                    	    	// 69
    ATTR_PARA_CONNECT_BORDER,                	    	// 70
    ATTR_PARA_DUMMY5,								              	// 71
    ATTR_PARA_DUMMY6,								              	// 72
    ATTR_PARA_DUMMY7,								              	// 73
    ATTR_PARA_DUMMY8,								              	// 74

    ATTR_FRM_FILL_ORDER,                           	// 75
    ATTR_FRM_FRM_SIZE,                             	// 76
    ATTR_FRM_PAPER_BIN,                            	// 77
    ATTR_FRM_LR_SPACE,                             	// 78
    ATTR_FRM_UL_SPACE,                             	// 79
    ATTR_FRM_PAGEDESC,                             	// 80
    ATTR_FRM_BREAK,                                	// 81
    ATTR_FRM_CNTNT,                                	// 82
    ATTR_FRM_HEADER,                               	// 83
    ATTR_FRM_FOOTER,                               	// 84
    ATTR_FRM_PRINT,                                	// 85
    ATTR_FRM_OPAQUE,                               	// 86
    ATTR_FRM_PROTECT,                              	// 87
    ATTR_FRM_SURROUND,                             	// 88
    ATTR_FRM_VERT_ORIENT,                          	// 89
    ATTR_FRM_HORI_ORIENT,                          	// 90
    ATTR_FRM_ANCHOR,                               	// 91
    ATTR_FRM_BACKGROUND,                           	// 92
    ATTR_FRM_BOX,                                  	// 93
    ATTR_FRM_SHADOW,                               	// 94
    ATTR_FRM_FRMMACRO,                             	// 95
    ATTR_FRM_COL,                                  	// 96
    ATTR_FRM_KEEP,                                 	// 97
    ATTR_FRM_URL,                                  	// 98
    ATTR_FRM_EDIT_IN_READONLY,                     	// 99
    ATTR_FRM_LAYOUT_SPLIT,                         	// 100
    ATTR_FRM_CHAIN,                                	// 101
    ATTR_FRM_TEXTGRID,                             	// 102
    ATTR_FRM_LINENUMBER  ,                         	// 103
    ATTR_FRM_FTN_AT_TXTEND,                        	// 104
    ATTR_FRM_END_AT_TXTEND,                        	// 105
    ATTR_FRM_COLUMNBALANCE,                        	// 106
    ATTR_FRM_FRAMEDIR,                             	// 107
    ATTR_FRM_HEADER_FOOTER_EAT_SPACING,            	// 108
    ATTR_FRM_FRMATTR_DUMMY9,                        // 109

    ATTR_GRF_MIRRORGRF,  	                 	      	// 110
    ATTR_GRF_CROPGRF,                       	      // 111
    ATTR_GRF_ROTATION,                      	      // 112
    ATTR_GRF_LUMINANCE,                     	      // 113
    ATTR_GRF_CONTRAST,                      	      // 114
    ATTR_GRF_CHANNELR,                      	      // 115
    ATTR_GRF_CHANNELG,                      	      // 116
    ATTR_GRF_CHANNELB,                      	      // 117
    ATTR_GRF_GAMMA,                         	      // 118
    ATTR_GRF_INVERT,                        	      // 119
    ATTR_GRF_TRANSPARENCY,                  	      // 120
    ATTR_GRF_DRAWMODE,                      	      // 121
    ATTR_GRF_DUMMY1,                        	      // 122
    ATTR_GRF_DUMMY2,                        	      // 123
    ATTR_GRF_DUMMY3,                        	      // 124
    ATTR_GRF_DUMMY4,                        	      // 125
    ATTR_GRF_DUMMY5,                        	      // 126

    ATTR_BOX_FORMAT,     	                 	      	// 127
    ATTR_BOX_FORMULA,                       	      // 128
    ATTR_BOX_VALUE,                         	      // 129

    // other
    ATTR_SC_USERDEF,
    ATTR_SC_HYPHENATE,
    ATTR_SC_HORJUSTIFY,
    ATTR_SC_INDENT,
    ATTR_SC_VERJUSTIFY,
    ATTR_SC_ORIENTATION,
    ATTR_SC_ROTATE_VALUE,
    ATTR_SC_ROTATE_MODE,
    ATTR_SC_VERTICAL_ASIAN,
    ATTR_SC_WRITINGDIR,
    ATTR_SC_LINEBREAK,
    ATTR_SC_MARGIN,
    ATTR_SC_MERGE,
    ATTR_SC_MERGE_FLAG,
    ATTR_SC_VALUE_FORMAT,
    ATTR_SC_LANGUAGE_FORMAT,
    ATTR_SC_PROTECTION,
    ATTR_SC_BORDER,
    ATTR_SC_BORDER_INNER,
    ATTR_SC_VALIDDATA,
    ATTR_SC_CONDITIONAL,
    ATTR_SC_PATTERN, // TODO
    ATTR_SC_PAGE,
    ATTR_SC_PAGE_PAPERTRAY,
    ATTR_SC_PAGE_SIZE,
    ATTR_SC_PAGE_MAXSIZE,
    ATTR_SC_PAGE_HORCENTER,
    ATTR_SC_PAGE_VERCENTER,
    ATTR_SC_PAGE_ON,
    ATTR_SC_PAGE_DYNAMIC,
    ATTR_SC_PAGE_SHARED,
    ATTR_SC_PAGE_NOTES,
    ATTR_SC_PAGE_GRID,
    ATTR_SC_PAGE_HEADERS,
    ATTR_SC_PAGE_CHARTS,
    ATTR_SC_PAGE_OBJECTS,
    ATTR_SC_PAGE_DRAWINGS,
    ATTR_SC_PAGE_TOPDOWN,
    ATTR_SC_PAGE_SCALE,
    ATTR_SC_PAGE_SCALETOPAGES,
    ATTR_SC_PAGE_FIRSTPAGENO,
    ATTR_SC_PAGE_PRINTAREA,
    ATTR_SC_PAGE_REPEATROW,
    ATTR_SC_PAGE_REPEATCOL,
    ATTR_SC_PAGE_PRINTTABLES,
    ATTR_SC_PAGE_HEADERLEFT,
    ATTR_SC_PAGE_FOOTERLEFT,
    ATTR_SC_PAGE_HEADERRIGHT,
    ATTR_SC_PAGE_FOOTERRIGHT,
    ATTR_SC_PAGE_HEADERSET,
    ATTR_SC_PAGE_FOOTERSET,
    ATTR_SC_PAGE_FORMULAS,
    ATTR_SC_PAGE_NULLVALS,

    ATTR_EE_PARA_XMLATTRIBS,
    ATTR_EE_PARA_ASIANCJKSPACING,
    ATTR_EE_PARA_NUMBULLET,
    ATTR_EE_PARA_BULLETSTATE,
    ATTR_EE_PARA_OUTLLR_SPACE,
    ATTR_EE_PARA_OUTLLEVEL,
    ATTR_EE_PARA_BULLET,
    ATTR_EE_CHR_SCALEW,
    ATTR_EE_CHR_RUBI_DUMMY,
    ATTR_EE_CHR_XMLATTRIBS,
    ATTR_EE_FEATURE_TAB,
    ATTR_EE_FEATURE_LINEBR,
    ATTR_EE_FEATURE_FIELD,

    ATTR_SCH_DATADESCR_DESCR,
    ATTR_SCH_DATADESCR_SHOW_SYM,
    ATTR_SCH_LEGEND_POS,
    ATTR_SCH_TEXT_ORIENT,
    ATTR_SCH_TEXT_ORDER,

    ATTR_SCH_X_AXIS_AUTO_MIN,
    ATTR_SCH_X_AXIS_MIN,
    ATTR_SCH_X_AXIS_AUTO_MAX,
    ATTR_SCH_X_AXIS_MAX,
    ATTR_SCH_X_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_X_AXIS_STEP_MAIN,
    ATTR_SCH_X_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_X_AXIS_STEP_HELP,
    ATTR_SCH_X_AXIS_LOGARITHM,
    ATTR_SCH_X_AXIS_AUTO_ORIGIN,
    ATTR_SCH_X_AXIS_ORIGIN,
    ATTR_SCH_Y_AXIS_AUTO_MIN,
    ATTR_SCH_Y_AXIS_MIN,
    ATTR_SCH_Y_AXIS_AUTO_MAX,
    ATTR_SCH_Y_AXIS_MAX,
    ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_Y_AXIS_STEP_MAIN,
    ATTR_SCH_Y_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_Y_AXIS_STEP_HELP,
    ATTR_SCH_Y_AXIS_LOGARITHM,
    ATTR_SCH_Y_AXIS_AUTO_ORIGIN,
    ATTR_SCH_Y_AXIS_ORIGIN,
    ATTR_SCH_Z_AXIS_AUTO_MIN,
    ATTR_SCH_Z_AXIS_MIN,
    ATTR_SCH_Z_AXIS_AUTO_MAX,
    ATTR_SCH_Z_AXIS_MAX,
    ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_Z_AXIS_STEP_MAIN,
    ATTR_SCH_Z_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_Z_AXIS_STEP_HELP,
    ATTR_SCH_Z_AXIS_LOGARITHM,
    ATTR_SCH_Z_AXIS_AUTO_ORIGIN,
    ATTR_SCH_Z_AXIS_ORIGIN,

    ATTR_SCH_AXISTYPE,
    ATTR_SCH_DUMMY0,
    ATTR_SCH_DUMMY1,
    ATTR_SCH_DUMMY2,
    ATTR_SCH_DUMMY3,
    ATTR_SCH_DUMMY_END,

    ATTR_SCH_STAT_AVERAGE,
    ATTR_SCH_STAT_KIND_ERROR,
    ATTR_SCH_STAT_PERCENT,
    ATTR_SCH_STAT_BIGERROR,
    ATTR_SCH_STAT_CONSTPLUS,
    ATTR_SCH_STAT_CONSTMINUS,
    ATTR_SCH_STAT_REGRESSTYPE,
    ATTR_SCH_STAT_INDICATE,
    ATTR_SCH_TEXT_DEGREES,
    ATTR_SCH_TEXT_OVERLAP,
    ATTR_SCH_TEXT_DUMMY0,
    ATTR_SCH_TEXT_DUMMY1,
    ATTR_SCH_TEXT_DUMMY2,
    ATTR_SCH_TEXT_DUMMY3,

    ATTR_SCH_STYLE_DEEP,
    ATTR_SCH_STYLE_3D,
    ATTR_SCH_STYLE_VERTICAL,
    ATTR_SCH_STYLE_BASETYPE,
    ATTR_SCH_STYLE_LINES,
    ATTR_SCH_STYLE_PERCENT,
    ATTR_SCH_STYLE_STACKED,
    ATTR_SCH_STYLE_SPLINES,
    ATTR_SCH_STYLE_SYMBOL,
    ATTR_SCH_STYLE_SHAPE,

    ATTR_SCH_AXIS,
    ATTR_SCH_AXIS_AUTO_MIN,
    ATTR_SCH_AXIS_MIN,
    ATTR_SCH_AXIS_AUTO_MAX,
    ATTR_SCH_AXIS_MAX,
    ATTR_SCH_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_AXIS_STEP_MAIN,
    ATTR_SCH_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_AXIS_STEP_HELP,
    ATTR_SCH_AXIS_LOGARITHM,
    ATTR_SCH_AXIS_AUTO_ORIGIN,
    ATTR_SCH_AXIS_ORIGIN,

    ATTR_SCH_AXIS_TICKS,
    ATTR_SCH_AXIS_NUMFMT,
    ATTR_SCH_AXIS_NUMFMTPERCENT,
    ATTR_SCH_AXIS_SHOWAXIS,
    ATTR_SCH_AXIS_SHOWDESCR,
    ATTR_SCH_AXIS_SHOWMAINGRID,
    ATTR_SCH_AXIS_SHOWHELPGRID,
    ATTR_SCH_AXIS_TOPDOWN,
    ATTR_SCH_AXIS_HELPTICKS,

    ATTR_SCH_AXIS_DUMMY0,
    ATTR_SCH_AXIS_DUMMY1,
    ATTR_SCH_AXIS_DUMMY2,
    ATTR_SCH_AXIS_DUMMY3,
    ATTR_SCH_BAR_OVERLAP,
    ATTR_SCH_BAR_GAPWIDTH,

    ATTR_SCH_SYMBOL_BRUSH,
    ATTR_SCH_STOCK_VOLUME,
    ATTR_SCH_STOCK_UPDOWN,
    ATTR_SCH_SYMBOL_SIZE,
    ATTR_SCH_USER_DEFINED_ATTR
  };
  //! constructor
  StarAttribute();
  //! destructor
  virtual ~StarAttribute();


  //! try to read an attribute
  bool readAttribute(StarZone &zone, int which, int vers, long endPos, StarDocument &document);
  //! try to read a brush
  bool readBrushItem(StarZone &zone, int nVers, long endPos, libstoff::DebugStream &f);

protected:
  //
  // data
  //
private:
  //! the state
  shared_ptr<StarAttributeInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
