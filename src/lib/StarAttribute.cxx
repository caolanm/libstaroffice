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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <librevenge/librevenge.h>

#include "SDWParser.hxx"
#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"
#include "StarBitmap.hxx"
#include "StarDocument.hxx"
#include "StarFileManager.hxx"
#include "StarZone.hxx"

#include "StarAttribute.hxx"

/** Internal: the structures of a StarAttribute */
namespace StarAttributeInternal
{
//! void attribute of StarAttributeInternal
class StarAttributeVoid : public StarAttribute
{
public:
  //! constructor
  StarAttributeVoid(Type type, std::string const &debugName) : StarAttribute(type, debugName)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarAttributeVoid(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int /*vers*/, long /*endPos*/, StarDocument &/*document*/) const
  {
    STOFFInputStreamPtr input=zone.input();
    libstoff::DebugFile &ascFile=zone.ascii();
    libstoff::DebugStream f;
    f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << ",";
    ascFile.addPos(input->tell());
    ascFile.addNote(f.str().c_str());
    return true;
  }

};

////////////////////////////////////////
//! Internal: the state of a StarAttribute
struct State {
  //! constructor
  State() : m_whichToAttributeMap()
  {
    initAttributeMap();
  }
  //! init the attribute map list
  void initAttributeMap();
  //! a map which to an attribute
  std::map<int, shared_ptr<StarAttribute> > m_whichToAttributeMap;
};

void State::initAttributeMap()
{
  std::stringstream s;
  for (int type=StarAttribute::XATTR_LINERESERVED2; type<=StarAttribute::XATTR_LINERESERVED_LAST; ++type) {
    s.str("");
    s << "line[reserved" << type-StarAttribute::XATTR_LINERESERVED2+2 << "]";
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::Type(type), s.str()));
  }
  for (int type=StarAttribute::XATTR_FILLRESERVED2; type<=StarAttribute::XATTR_FILLRESERVED_LAST; ++type) {
    s.str("");
    s << "fill[reserved" << type-StarAttribute::XATTR_FILLRESERVED2+2 << "]";
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::Type(type), s.str()));
  }
  for (int type=StarAttribute::XATTR_FTRESERVED2; type<=StarAttribute::XATTR_FTRESERVED_LAST; ++type) {
    s.str("");
    s << "form[reserved" << type-StarAttribute::XATTR_FTRESERVED2+2 << "]";
    m_whichToAttributeMap[type]=shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::Type(type), s.str()));
  }
}

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarAttributeManager::StarAttributeManager() : m_state(new StarAttributeInternal::State)
{
}

StarAttributeManager::~StarAttributeManager()
{
}

bool StarAttributeManager::readAttribute(StarZone &zone, int nWhich, int nVers, long lastPos, StarDocument &document)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";

  long pos=input->tell();
  if (m_state->m_whichToAttributeMap.find(nWhich)!=m_state->m_whichToAttributeMap.end() &&
      m_state->m_whichToAttributeMap.find(nWhich)->second) {
    shared_ptr<StarAttribute> attrib=m_state->m_whichToAttributeMap.find(nWhich)->second->create();
    if (!attrib || !attrib->read(zone, nVers, lastPos, document)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read an attribute\n"));
      f << "###bad";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    return true;
  }

  int val;
  switch (nWhich) {
  case StarAttribute::ATTR_CHR_CASEMAP:
    f << "chrAtrCaseMap=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_CHARSETCOLOR: {
    f << "chrAtrCharSetColor=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a color\n"));
      f << "###aColor,";
      break;
    }
    if (!color.isBlack())
      f << color << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_COLOR: {
    f << "chrAtrColor,";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a color\n"));
      f << "###aColor,";
      break;
    }
    if (!color.isBlack())
      f << color << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_CONTOUR:
    f << "chrAtrContour=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_CROSSEDOUT:
    f << "chrAtrCrossedOut=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_ESCAPEMENT:
    f << "chrAtrEscapement=" << input->readULong(1) << ",";
    f << "nEsc=" << input->readLong(2) << ",";
    break;
  case StarAttribute::ATTR_CHR_FONT:
  case StarAttribute::ATTR_CHR_CJK_FONT:
  case StarAttribute::ATTR_CHR_CTL_FONT: {
    if (nWhich==StarAttribute::ATTR_CHR_FONT)
      f << "chrAtrFont,";
    else if (nWhich==StarAttribute::ATTR_CHR_CJK_FONT)
      f << "chrAtrCJKFont,";
    else
      f << "chrAtrCTLFont,";
    f << "family=" << input->readULong(1) << ",";
    f << "ePitch=" << input->readULong(1) << ",";
    int encoding=(int) input->readULong(1);
    f << "eTextEncoding=" << encoding << ",";
    librevenge::RVNGString fName, string;
    if (!zone.readString(fName)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the name\n"));
      f << "###aName,";
      break;
    }
    if (!fName.empty())
      f << "aName=" << fName.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the style\n"));
      f << "###aStyle,";
      break;
    }
    if (!string.empty())
      f << "aStyle=" << string.cstr() << ",";
    if (encoding!=10 && fName=="StarBats" && input->tell()<lastPos) {
      if (input->readULong(4)==0xFE331188) {
        // reread data in unicode
        if (!zone.readString(fName)) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the name\n"));
          f << "###aName,";
          break;
        }
        if (!fName.empty())
          f << "aNameUni=" << fName.cstr() << ",";
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the style\n"));
          f << "###aStyle,";
          break;
        }
        if (!string.empty())
          f << "aStyleUni=" << string.cstr() << ",";
      }
      else input->seek(-3, librevenge::RVNG_SEEK_CUR);
    }
    break;
  }
  case StarAttribute::ATTR_CHR_FONTSIZE:
  case StarAttribute::ATTR_CHR_CJK_FONTSIZE:
  case StarAttribute::ATTR_CHR_CTL_FONTSIZE:
    if (nWhich==StarAttribute::ATTR_CHR_FONTSIZE)
      f << "chrAtrFontSize,";
    else if (nWhich==StarAttribute::ATTR_CHR_CJK_FONTSIZE)
      f << "chrAtrCJKFontSize,";
    else
      f << "chrAtrCTLFontSize,";
    f << "size=" << input->readULong(2) << ",";
    f << "nProp=" << input->readULong((nVers>=1) ? 2 : 1) << ",";
    if (nVers>=2) f << "nPropUnit=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_CHR_KERNING:
    f << "chrAtrKerning=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_CHR_LANGUAGE:
  case StarAttribute::ATTR_CHR_CJK_LANGUAGE:
  case StarAttribute::ATTR_CHR_CTL_LANGUAGE:
  case StarAttribute::ATTR_SC_LANGUAGE_FORMAT:
    f << (nWhich==StarAttribute::ATTR_CHR_LANGUAGE ? "chrAtrLanguage": nWhich==StarAttribute::ATTR_CHR_CJK_LANGUAGE ? "chrAtrCJKLanguage" :
          nWhich==StarAttribute::ATTR_CHR_CTL_LANGUAGE ? "chrAtrCTLLanguage" : "scLanguage");
    f << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_CHR_POSTURE:
  case StarAttribute::ATTR_CHR_CJK_POSTURE:
  case StarAttribute::ATTR_CHR_CTL_POSTURE:
    f << (nWhich==StarAttribute::ATTR_CHR_POSTURE ? "chrAtrPosture": nWhich==StarAttribute::ATTR_CHR_CJK_POSTURE ? "chrAtrCJKPosture" : "chrAtrCTLPosture");
    f << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_PROPORTIONALFONTSIZE:
    f << "chrAtrProportionFontSize,";
    f << "size=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_CHR_SHADOWED:
    f << "chrAtrShadowed=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_UNDERLINE:
    f << "chrAtrUnderline=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_WEIGHT:
  case StarAttribute::ATTR_CHR_CJK_WEIGHT:
  case StarAttribute::ATTR_CHR_CTL_WEIGHT:
    f << (nWhich==StarAttribute::ATTR_CHR_WEIGHT ? "chrAtrWeight" : nWhich==StarAttribute::ATTR_CHR_CJK_WEIGHT ? "chrAtrCJKWeight" : "chrAtrCTLWeight");
    f << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_WORDLINEMODE:
    f << "chrAtrWordlineMode=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_AUTOKERN:
    f << "chrAtrAutoKern=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_BLINK:
    f << "chrAtrBlink=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_NOHYPHEN:
    f << "chrAtrNoHyphen=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_NOLINEBREAK:
    f << "chrAtrNoLineBreak=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_BACKGROUND:
  case StarAttribute::ATTR_FRM_BACKGROUND:
  case StarAttribute::ATTR_SCH_SYMBOL_BRUSH:
    f << (nWhich==StarAttribute::ATTR_CHR_BACKGROUND ? "chrAtrBackground" :
          nWhich==StarAttribute::ATTR_FRM_BACKGROUND ? "background" : "symbol[brush]") << "=" << input->readULong(1) << ",";
    if (!readBrushItem(zone, nVers, lastPos, f)) break;
    break;
  case StarAttribute::ATTR_CHR_ROTATE:
    f << "chrAtrRotate,";
    f << "nVal=" << input->readULong(2) << ",";
    f << "b=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_EMPHASIS_MARK:
    f << "chrAtrEmphasisMark=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_CHR_TWO_LINES: { // checkme
    f << "chrAtrTwoLines=" << input->readULong(1) << ",";
    f << "nStart[unicode]=" << input->readULong(2) << ",";
    f << "nEnd[unicode]=" << input->readULong(2) << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_SCALEW:
  case StarAttribute::ATTR_EE_CHR_SCALEW:
    f << (nWhich==StarAttribute::ATTR_CHR_SCALEW ? "chrAtrScaleW":"eeScaleW") << "=" << input->readULong(2) << ",";
    if (input->tell()<lastPos) {
      f << "nVal=" << input->readULong(2) << ",";
      f << "test=" << input->readULong(2) << ",";
    }
    break;
  case StarAttribute::ATTR_CHR_RELIEF:
    f << "chrAtrRelief=" << input->readULong(2);
    break;
  case StarAttribute::ATTR_CHR_DUMMY1:
    f << "chrAtrDummy1=" << input->readULong(1) << ",";
    break;

  // text attribute
  case StarAttribute::ATTR_TXT_INETFMT: {
    f << "textAtrInetFmt,";
    // SwFmtINetFmt::Create
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find string\n"));
      f << "###url,";
      break;
    }
    if (!string.empty())
      f << "url=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find string\n"));
      f << "###targert,";
      break;
    }
    if (!string.empty())
      f << "target=" << string.cstr() << ",";
    f << "nId1=" << input->readULong(2) << ",";
    f << "nId2=" << input->readULong(2) << ",";
    int nCnt=(int) input->readULong(2);
    bool ok=true;
    f << "libMac=[";
    for (int i=0; i<nCnt; ++i) {
      if (input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a libmac name\n"));
        f << "###libname,";
        ok=false;
        break;
      }
      if (!zone.readString(string)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
        ok=false;
        break;
      }
      else if (!string.empty())
        f << string.cstr() << ":";
      if (!zone.readString(string)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
        ok=false;
        break;
      }
      else if (!string.empty())
        f << string.cstr();
      f << ",";
    }
    f << "],";
    if (!ok) break;
    if (nVers>=1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find string\n"));
        f << "###aName1,";
        break;
      }
      if (!string.empty())
        f << "aName1=" << string.cstr() << ",";
    }
    if (nVers>=2) {
      nCnt=(int) input->readULong(2);
      f << "libMac2=[";
      for (int i=0; i<nCnt; ++i) {
        f << "nCurKey=" << input->readULong(2) << ",";
        if (input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a libmac name\n"));
          f << "###libname2,";
          break;
        }
        if (!zone.readString(string)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
          break;
        }
        else if (!string.empty())
          f << string.cstr() << ":";
        if (!zone.readString(string)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
          break;
        }
        else if (!string.empty())
          f << string.cstr();
        f << "nScriptType=" << input->readULong(2) << ",";
      }
      f << "],";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_DUMMY4:
    f << "textAtrDummy4=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_TXT_REFMARK: {
    f << "textAtrRefMark,";
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aName\n"));
      f << "###aName,";
      break;
    }
    if (!string.empty())
      f << "aName=" << string.cstr() << ",";
    break;
  }
  case StarAttribute::ATTR_TXT_TOXMARK: {
    f << "textAtrToXMark,";
    int cType=(int) input->readULong(1);
    f << "cType=" << cType << ",";
    f << "nLevel=" << input->readULong(2) << ",";
    librevenge::RVNGString string;
    int nStringId=0xFFFF;
    if (nVers<1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aTypeName\n"));
        f << "###aTypeName,";
        break;
      }
      if (!string.empty())
        f << "aTypeName=" << string.cstr() << ",";
    }
    else {
      nStringId=(int) input->readULong(2);
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
    }
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aAltText\n"));
      f << "###aAltText,";
      break;
    }
    if (!string.empty())
      f << "aAltText=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aPrimKey\n"));
      f << "###aPrimKey,";
      break;
    }
    if (!string.empty())
      f << "aPrimKey=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aSecKey\n"));
      f << "###aSecKey,";
      break;
    }
    if (!string.empty())
      f << "aSecKey=" << string.cstr() << ",";
    if (nVers>=2) {
      cType=(int) input->readULong(1);
      f << "cType=" << cType << ",";
      nStringId=(int) input->readULong(2);
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
      f << "cFlags=" << input->readULong(1) << ",";
    }
    if (nVers>=1 && nStringId!=0xFFFF) {
      if (!zone.getPoolName(nStringId, string)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a nId name\n"));
        f << "###nId=" << nStringId << ",";
      }
      else
        f << string.cstr() << ",";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_CHARFMT:
    f << "textAtrCharFmt=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_TXT_DUMMY5:
    f << "textAtrDummy5=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_TXT_CJK_RUBY:
    f << "textAtrCJKRuby=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_TXT_UNKNOWN_CONTAINER:
  case StarAttribute::ATTR_SC_USERDEF:
  case StarAttribute::ATTR_EE_PARA_XMLATTRIBS:
  case StarAttribute::ATTR_EE_CHR_XMLATTRIBS:
  case StarAttribute::ATTR_SCH_USER_DEFINED_ATTR:
    // call SfxPoolItem::Create which does nothing
    f << (nWhich==StarAttribute::ATTR_TXT_UNKNOWN_CONTAINER ?  "textAtrUnknownContainer" :
          nWhich==StarAttribute::ATTR_SC_USERDEF ? "scUserDef" :
          nWhich==StarAttribute::ATTR_EE_PARA_XMLATTRIBS ? "eeParaXmlAttr" :
          nWhich==StarAttribute::ATTR_EE_CHR_XMLATTRIBS ? "eeCharXmlAttr" : "schUserDef") << ",";
    break;
  case StarAttribute::ATTR_TXT_DUMMY6:
    f << "textAtrDummy6" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_TXT_DUMMY7:
    f << "textAtrDummy7" << input->readULong(1) << ",";
    break;

  // field...
  case StarAttribute::ATTR_TXT_FIELD: {
    f << "textAtrField,";
    SWFieldManager fieldManager;
    fieldManager.readField(zone);
    break;
  }
  case StarAttribute::ATTR_TXT_FLYCNT: {
    f << "textAtrFlycnt,";
    SWFormatManager formatManager;
    if (input->peek()=='o')
      formatManager.readSWFormatDef(zone,'o', document);
    else
      formatManager.readSWFormatDef(zone,'l', document);
    break;
  }
  case StarAttribute::ATTR_TXT_FTN: {
    f << "textAtrFtn,";
    // sw_sw3npool.cxx SwFmtFtn::Create
    f << "number1=" << input->readULong(2) << ",";
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the aNumber\n"));
      f << "###aNumber,";
      break;
    }
    if (!string.empty())
      f << "aNumber=" << string.cstr() << ",";
    // no sure, find this attribute once with a content here, so ...
    if (!document.getSDWParser()->readSWContent(zone, document)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the content\n"));
      f << "###aContent,";
      break;
    }
    if (nVers>=1) {
      uint16_t nSeqNo;
      *input >> nSeqNo;
      if (nSeqNo) f << "nSeqNo=" << nSeqNo << ",";
    }
    if (nVers>=1) {
      uint8_t nFlags;
      *input >> nFlags;
      if (nFlags) f << "nFlags=" << nFlags << ",";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_SOFTHYPH: // ok no data
    f << "textAtrSoftHyph,";
    break;
  case StarAttribute::ATTR_TXT_HARDBLANK: // ok no data
    f << "textAtrHardBlank,";
    break;
  case StarAttribute::ATTR_TXT_DUMMY1:
    f << "textAtrDummy1" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_TXT_DUMMY2:
    f << "textAtrDummy2" << input->readULong(1) << ",";
    break;

  // paragraph attribute
  case StarAttribute::ATTR_PARA_LINESPACING:
    f << "parAtrLineSpacing,";
    f << "nPropSpace=" << input->readLong(1) << ",";
    f << "nInterSpace=" << input->readLong(2) << ",";
    f << "nHeight=" << input->readULong(2) << ",";
    f << "nRule=" << input->readULong(1) << ",";
    f << "nInterRule=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_ADJUST:
    f << "parAtrAdjust=" << input->readULong(1) << ",";
    if (nVers>=1) f << "nFlags=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_SPLIT:
    f << "parAtrSplit=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_ORPHANS:
    f << "parAtrOrphans,";
    f << "nLines="  << input->readLong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_WIDOWS:
    f << "parAtrWidows,";
    f << "nLines="  << input->readLong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_TABSTOP: {
    f << "parAtrTabStop,";
    int N=(int) input->readULong(1);
    if (input->tell()+7*N>lastPos) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: N is too big\n"));
      f << "###N=" << N << ",";
      N=int(lastPos-input->tell())/7;
    }
    f << "tabs=[";
    for (int i=0; i<N; ++i) {
      int nPos=(int) input->readLong(4);
      f << nPos << "->" << input->readLong(1) << ":" << input->readLong(1) << ":" << input->readLong(1) << ",";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_PARA_HYPHENZONE:
    f << "parAtrHyphenZone=" << input->readLong(1) << ",";
    f << "bHyphenPageEnd=" << input->readLong(1) << ",";
    f << "nMinLead=" << input->readLong(1) << ",";
    f << "nMinTail=" << input->readLong(1) << ",";
    f << "nMaxHyphen=" << input->readLong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_DROP:
    f << "parAtrDrop,";
    f << "nFmt=" << input->readULong(2) << ",";
    f << "nLines1=" << input->readULong(2) << ",";
    f << "nChars1=" << input->readULong(2) << ",";
    f << "nDistance1=" << input->readULong(2) << ",";
    if (nVers>=1)
      f << "bWhole=" << input->readULong(1) << ",";
    else {
      f << "nX=" << input->readULong(2) << ",";
      f << "nY=" << input->readULong(2) << ",";
    }
    break;
  case StarAttribute::ATTR_PARA_REGISTER:
    f << "parAtrRegister=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_NUMRULE: {
    f << "parAtrNumRule,";
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the sTmp\n"));
      f << "###sTmp,";
      break;
    }
    if (!string.empty())
      f << "sTmp=" << string.cstr() << ",";
    if (nVers>0)
      f << "nPoolId=" << input->readULong(2) << ",";
    break;
  }
  case StarAttribute::ATTR_PARA_SCRIPTSPACE:
  case StarAttribute::ATTR_EE_PARA_ASIANCJKSPACING:
    f << (nWhich==StarAttribute::ATTR_PARA_SCRIPTSPACE ? "parAtrScriptSpace" : "parAsianScriptSpace")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_HANGINGPUNCTUATION:
    f << "parAtrHangingPunctuation=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_FORBIDDEN_RULES:
    f << "parAtrForbiddenRules=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_VERTALIGN:
    f << "parAtrVertAlign=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_PARA_SNAPTOGRID:
    f << "parAtrSnapToGrid=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_CONNECT_BORDER:
    f << "parAtrConnectBorder=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_DUMMY5:
    f << "parAtrDummy5" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_DUMMY6:
    f << "parAtrDummy6" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_DUMMY7:
    f << "parAtrDummy7" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_PARA_DUMMY8:
    f << "parAtrDummy8" << input->readULong(1) << ",";
    break;

  // frame parameter
  case StarAttribute::ATTR_FRM_FILL_ORDER:
    f << "fillOrder=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_FRM_SIZE:
    f << "frmSize,";
    f << "sizeType=" << input->readULong(1) << ",";
    f << "width=" << input->readULong(4) << ",";
    f << "height=" << input->readULong(4) << ",";
    if (nVers>1)
      f << "percent=" << input->readULong(1) << "x"  << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_PAPER_BIN:
    f << "paperBin=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_LR_SPACE:
  case StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE:
    f << (nWhich==StarAttribute::ATTR_FRM_LR_SPACE ? "lrSpace" : "eeOutLrSpace") << ",";
    f << "left=" << input->readULong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "right=" << input->readULong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "firstLine=" << input->readLong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    if (nVers>=2)
      f << "textLeft=" << input->readLong(2) << ",";
    if (nVers>=3) {
      long marker=(long) input->readULong(4);
      if (marker==0x599401FE)
        f << "firstLine[bullet]=" << input->readULong(2);
      else if (input->tell()==lastPos+3) // normally end by 0
        input->seek(-3, librevenge::RVNG_SEEK_CUR);
      else
        input->seek(-4, librevenge::RVNG_SEEK_CUR);
    }
    break;
  case StarAttribute::ATTR_FRM_UL_SPACE:
    f << "ulSpace,";
    f << "upper=" << input->readULong(2);
    val=(int) input->readULong(nVers==1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "lower=" << input->readULong(2);
    val=(int) input->readULong(nVers==1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    break;
  case StarAttribute::ATTR_FRM_PAGEDESC:
    f << "pageDesc,";
    if (nVers<1)
      f << "bAutor=" << input->readULong(1) << ",";
    if (nVers< 2)
      f << "nOff=" << input->readULong(2) << ",";
    else {
      unsigned long nOff;
      if (!input->readCompressedULong(nOff)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read nOff\n"));
        f << "###nOff,";
        break;
      }
      f << "nOff=" << nOff << ",";
    }
    f << "nIdx=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_FRM_BREAK:
    f << "pageBreak=" << input->readULong(1) << ",";
    if (nVers<1) input->seek(1, librevenge::RVNG_SEEK_CUR); // dummy
    break;
  case StarAttribute::ATTR_FRM_CNTNT: // checkme
    f << "pageCntnt,";
    while (input->tell()<lastPos) {
      long actPos=input->tell();
      if (input->peek()!='N' || !document.getSDWParser()->readSWContent(zone, document) || input->tell()<=actPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: find unknown pageCntnt child\n"));
        f << "###child";
        break;
      }
    }
    break;
  case StarAttribute::ATTR_FRM_HEADER:
  case StarAttribute::ATTR_FRM_FOOTER: {
    f << (nWhich==StarAttribute::ATTR_FRM_HEADER ? "header" : "footer") << ",";
    f << "active=" << input->readULong(1) << ",";
    long actPos=input->tell();
    if (actPos==lastPos)
      break;
    SWFormatManager formatManager;
    formatManager.readSWFormatDef(zone,'r',document);
    break;
  }
  case StarAttribute::ATTR_FRM_PRINT:
    f << "print=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_OPAQUE:
    f << "opaque=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_PROTECT:
    f << "protect,";
    val=(int) input->readULong(1);
    if (val&1) f << "pos[protect],";
    if (val&2) f << "size[protect],";
    if (val&4) f << "cntnt[protect],";
    break;
  case StarAttribute::ATTR_FRM_SURROUND:
    f << "surround=" << input->readULong(1) << ",";
    if (nVers<5) f << "bGold=" << input->readULong(1) << ",";
    if (nVers>1) f << "bAnch=" << input->readULong(1) << ",";
    if (nVers>2) f << "bCont=" << input->readULong(1) << ",";
    if (nVers>3) f << "bOutside1=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_VERT_ORIENT:
  case StarAttribute::ATTR_FRM_HORI_ORIENT:
    f << (nWhich==StarAttribute::ATTR_FRM_VERT_ORIENT ? "vertOrient" : "horiOrient") << ",";
    f << "nPos=" << input->readULong(4) << ",";
    f << "nOrient=" << input->readULong(1) << ",";
    f << "nRel=" << input->readULong(1) << ",";
    if (nWhich==StarAttribute::ATTR_FRM_HORI_ORIENT && nVers>=1) f << "bToggle=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_ANCHOR:
    f << "anchor=" << input->readULong(1) << ",";
    if (nVers<1)
      f << "nId=" << input->readULong(2) << ",";
    else {
      unsigned long nId;
      if (!input->readCompressedULong(nId)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read nId\n"));
        f << "###nId,";
        break;
      }
      else
        f << "nId=" << nId << ",";
    }
    break;
  // ATTR_FRM_BACKGROUND see case StarAttribute::ATTR_CHR_BACKGROUND
  case StarAttribute::ATTR_FRM_BOX:
  case StarAttribute::ATTR_SC_BORDER: {
    f << (nWhich==StarAttribute::ATTR_FRM_BOX ? "box" : "scBorder") << ",";
    f << "nDist=" << input->readULong(2) << ",";
    int cLine=0;
    bool ok=true;
    while (input->tell()<lastPos) {
      cLine=(int) input->readULong(1);
      if (cLine>3) break;
      f << "[";
      STOFFColor color;
      if (!input->readColor(color)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a box's color\n"));
        f << "###color,";
        ok=false;
        break;
      }
      else if (!color.isBlack())
        f << "col=" << color << ",";
      f << "outline=" << input->readULong(2) << ",";
      f << "inline=" << input->readULong(2) << ",";
      f << "nDist=" << input->readULong(2) << ",";
      f << "],";
    }
    if (!ok) break;
    if (nVers>=1 && cLine&0x10) {
      f << "dist=[";
      for (int i=0; i<4; ++i) f << input->readULong(2) << ",";
      f << "],";
    }
    break;
  }
  case StarAttribute::ATTR_FRM_SHADOW: {
    f << "shadow,";
    f << "cLoc=" << input->readULong(1) << ",";
    f << "nWidth=" << input->readULong(2) << ",";
    f << "bTrans=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read color\n"));
      f << "###color,";
      break;
    }
    else if (!color.isWhite())
      f << "color=" << color << ",";
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read fill color\n"));
      f << "###fillcolor,";
      break;
    }
    else if (!color.isWhite())
      f << "fillcolor=" << color << ",";
    f << "style=" << input->readULong(1) << ",";
    break;
  }
  case StarAttribute::ATTR_FRM_FRMMACRO: { // macitem.cxx SvxMacroTableDtor::Read
    f << "frmMacro,";
    if (nVers>=1) {
      nVers=(uint16_t) input->readULong(2);
      f << "nVersion=" << nVers << ",";
    }
    int16_t nMacros;
    *input>>nMacros;
    f << "macros=[";
    for (int16_t i=0; i<nMacros; ++i) {
      uint16_t nCurKey, eType;
      bool ok=true;
      f << "[";
      *input>>nCurKey;
      if (nCurKey) f << "nCurKey=" << nCurKey << ",";
      for (int j=0; j<2; ++j) {
        librevenge::RVNGString text;
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a macro string\n"));
          f << "###string" << j << ",";
          ok=false;
          break;
        }
        else if (!text.empty())
          f << (j==0 ? "lib" : "mac") << "=" << text.cstr() << ",";
      }
      if (!ok) break;
      if (nVers>=1) {
        *input>>eType;
        if (eType) f << "eType=" << eType << ",";
      }
      f << "],";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_FRM_COL: {
    f << "col,";
    f << "nLineAdj=" << input->readULong(1) << ",";
    f << "bOrtho=" << input->readULong(1) << ",";
    f << "nLineHeight=" << input->readULong(1) << ",";
    f << "nGutterWidth=" << input->readLong(2) << ",";
    int nWishWidth=(int) input->readULong(2);
    f << "nWishWidth=" << nWishWidth << ",";
    f << "nPenStyle=" << input->readULong(1) << ",";
    f << "nPenWidth=" << input->readLong(2) << ",";
    f << "nPenRed=" << (input->readULong(2)>>8) << ",";
    f << "nPenGreen=" << (input->readULong(2)>>8) << ",";
    f << "nPenBlue=" << (input->readULong(2)>>8) << ",";
    int nCol=(int) input->readULong(2);
    f << "N=" << nCol << ",";
    if (nWishWidth==0)
      break;
    if (input->tell()+10*nCol>lastPos) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: nCol is bad\n"));
      f << "###N,";
      break;
    }
    f << "[";
    for (int i=0; i<nCol; ++i) {
      f << "[";
      f << "nWishWidth=" << input->readULong(2) << ",";
      f << "nLeft=" << input->readULong(2) << ",";
      f << "nUpper=" << input->readULong(2) << ",";
      f << "nRight=" << input->readULong(2) << ",";
      f << "nBottom=" << input->readULong(2) << ",";
      f << "],";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_FRM_KEEP:
    f << "keep=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_URL:
    f << "url,";
    if (!SDWParser::readSWImageMap(zone))
      break;
    if (nVers>=1) {
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the setName\n"));
        f << "###name1,";
        break;
      }
      else if (!text.empty())
        f << "name1=" << text.cstr() << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_EDIT_IN_READONLY:
    f << "editInReadOnly=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_LAYOUT_SPLIT:
    f << "layoutSplit=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_CHAIN:
    f << "chain,";
    if (nVers>0) {
      f << "prevIdx=" << input->readULong(2) << ",";
      f << "nextIdx=" << input->readULong(2) << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_TEXTGRID:
    f << "textgrid=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_LINENUMBER:
    f << "lineNumber,";
    f << "start=" << input->readULong(4) << ",";
    f << "count[lines]=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_FTN_AT_TXTEND:
  case StarAttribute::ATTR_FRM_END_AT_TXTEND:
    f << (nWhich==StarAttribute::ATTR_FRM_FTN_AT_TXTEND ? "ftnAtTextEnd" : "ednAtTextEnd") << "=" << input->readULong(1) << ",";
    if (nVers>0) {
      f << "offset=" << input->readULong(2) << ",";
      f << "fmtType=" << input->readULong(2) << ",";
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the prefix\n"));
        f << "###prefix,";
        break;
      }
      else if (!text.empty())
        f << "prefix=" << text.cstr() << ",";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the suffix\n"));
        f << "###suffix,";
        break;
      }
      else if (!text.empty())
        f << "suffix=" << text.cstr() << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_COLUMNBALANCE:
    f << "columnBalance=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_FRAMEDIR:
  case StarAttribute::ATTR_SC_WRITINGDIR:
    f << (nWhich==StarAttribute::ATTR_FRM_FRAMEDIR ? "frameDir" : "ScWritingDir") << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_FRM_HEADER_FOOTER_EAT_SPACING:
    f << "headerFooterEatSpacing=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_FRMATTR_DUMMY9:
    f << "frmAtrDummy9" << input->readULong(1) << ",";
    break;
  // graphic attribute
  case StarAttribute::ATTR_GRF_MIRRORGRF:
    f << "grfMirrorGrf=" << input->readULong(1) << ",";
    if (nVers>0) f << "nToggle=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_CROPGRF:
    f << "grfCropGrf,";
    f << "top=" << input->readLong(4) << ",";
    f << "left=" << input->readLong(4) << ",";
    f << "right=" << input->readLong(4) << ",";
    f << "bottom=" << input->readLong(4) << ",";
    break;
  // no saved ?
  case StarAttribute::ATTR_GRF_ROTATION:
    f << "grfRotation,";
    break;
  case StarAttribute::ATTR_GRF_LUMINANCE:
    f << "grfLuminance,";
    break;
  case StarAttribute::ATTR_GRF_CONTRAST:
    f << "grfContrast,";
    break;
  case StarAttribute::ATTR_GRF_CHANNELR:
    f << "grfChannelR,";
    break;
  case StarAttribute::ATTR_GRF_CHANNELG:
    f << "grfChannelG,";
    break;
  case StarAttribute::ATTR_GRF_CHANNELB:
    f << "grfChannelB,";
    break;
  case StarAttribute::ATTR_GRF_GAMMA:
    f << "grfGamma,";
    break;
  case StarAttribute::ATTR_GRF_INVERT:
    f << "grfInvert,";
    break;
  case StarAttribute::ATTR_GRF_TRANSPARENCY:
    f << "grfTransparency,";
    break;
  case StarAttribute::ATTR_GRF_DRAWMODE:
    f << "grfDrawMode,";
    break;
  case StarAttribute::ATTR_GRF_DUMMY1:
    f << "grfDummy1" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_DUMMY2:
    f << "grfDummy2" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_DUMMY3:
    f << "grfDummy3" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_DUMMY4:
    f << "grfDummy4" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_DUMMY5:
    f << "grfDummy5" << input->readULong(1) << ",";
    break;

  case StarAttribute::ATTR_BOX_FORMAT:
    f << "boxFormat=" << input->readULong(1) << ",";
    f << "nTmp=" << input->readULong(4) << ",";
    break;
  case StarAttribute::ATTR_BOX_FORMULA: {
    f << "boxFormula,";
    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the formula\n"));
      f << "###formula,";
      break;
    }
    else if (!text.empty())
      f << "formula=" << text.cstr() << ",";
    break;
  }
  case StarAttribute::ATTR_BOX_VALUE:
    f << "boxAtrValue,";
    if (nVers==0) {
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the dValue\n"));
        f << "###dValue,";
        break;
      }
      else if (!text.empty())
        f << "dValue=" << text.cstr() << ",";
    }
    else {
      double res;
      bool isNan;
      if (!input->readDoubleReverted8(res, isNan)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a double\n"));
        f << "##value,";
      }
      else if (res<0||res>0)
        f << "value=" << res << ",";
    }
    break;

  case StarAttribute::ATTR_SC_HYPHENATE:
    f << "scHyphenate=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_HORJUSTIFY:
  case StarAttribute::ATTR_SC_VERJUSTIFY:
    // algitem.cxx Svx{Hor,Ver}JustifyItem::Create
    f << (nWhich==StarAttribute::ATTR_SC_HORJUSTIFY ? "scHorJustify" : "scVerJustify") << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_INDENT:
    f << "scIndent=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_ORIENTATION:
    // algitem.cxx SvxOrientationItem::Create
    f << "scOrientation=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_ROTATE_VALUE:
    f << "scRotateValue=" << input->readLong(4) << ",";
    break;
  case StarAttribute::ATTR_SC_ROTATE_MODE:
    // rotmodit.cxx SvxRotateModeItem::Create
    f << "scRotateMode=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_VERTICAL_ASIAN:
    f << "scVerticalAsian=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_LINEBREAK:
    f << "scLineBreak=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_MARGIN:
    // algItem SvxMarginItem::Create
    f << "scMargin,";
    f << "top=" << input->readLong(2) << ",";
    f << "left=" << input->readLong(2) << ",";
    f << "right=" << input->readLong(2) << ",";
    f << "bottom=" << input->readLong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_MERGE:
    // sc_attrib.cxx ScMergeAttr::Create
    f << "scMerge,";
    f << "nCol=" << input->readLong(2) << ",";
    f << "nRow=" << input->readLong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_MERGE_FLAG:
    f << "scMergeFlag=" << input->readLong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_VALUE_FORMAT:
    f << "scValueFormat=" << input->readULong(4) << ",";
    break;
  case StarAttribute::ATTR_SC_PROTECTION:
    // sc_attrib.cxx ScProtectionAttr::Create
    f << "scProtection,";
    f << "bProtect=" << input->readLong(1) << ",";
    f << "bHFormula=" << input->readLong(1) << ",";
    f << "bHCell=" << input->readLong(1) << ",";
    f << "bHPrint=" << input->readLong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_BORDER_INNER: {
    // frmitem.cxx SvxBoxInfoItem::Create
    f << "scBorderInner,";
    int cFlags=(int) input->readULong(1);
    if (cFlags&1) f << "setTable,";
    if (cFlags&2) f << "setDist,";
    if (cFlags&4) f << "setMinDist,";
    f << "defDist=" << input->readULong(2) << ",";
    int n=0;
    while (input->tell()<lastPos) {
      int cLine=(int) input->readLong(1);
      if (cLine>1) break;
      f << "line" << n++ <<"=[";
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      else if (!col.isBlack())
        f << "col=" << col << ",";
      f << "out=" << input->readLong(2) << ",";
      f << "in=" << input->readLong(2) << ",";
      f << "dist=" << input->readLong(2) << ",";
      f << "],";
    }
    break;
  }
  case StarAttribute::ATTR_SC_VALIDDATA:
  case StarAttribute::ATTR_SC_CONDITIONAL:
    f << (nWhich==StarAttribute::ATTR_SC_VALIDDATA ? "scValidData" : "scConditional") << input->readULong(4) << ",";
    break;
  case StarAttribute::ATTR_SC_PATTERN: {
    f << "pattern,";
    // sc_patattr.cxx ScPatternAttr::Create
    bool hasStyle;
    *input>>hasStyle;
    if (hasStyle) {
      f << "hasStyle,";
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a name\n"));
        f << "###name,";
        break;
      }
      else if (!text.empty())
        f << "famDummy=" << text.cstr() << ",";
    }
    // TODO
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: reading a pattern item is not implemented\n"));
      first=false;
    }
    f << "##";
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    break;
  }
  case StarAttribute::ATTR_SC_PAGE: {
    // svx_pageitem.cxx SvxPageItem::Create
    f << "page,";
    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a name\n"));
      f << "###name,";
      break;
    }
    else if (!text.empty())
      f << "name=" << text.cstr() << ",";
    f << "type=" << input->readULong(1) << ",";
    f << "bLand="<< input->readULong(1) << ",";
    f << "nUse=" << input->readULong(2) << ",";
    break;
  }
  case StarAttribute::ATTR_SC_PAGE_PAPERTRAY:
    f << "paper[tray]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_SIZE:
  case StarAttribute::ATTR_SC_PAGE_MAXSIZE:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_SIZE ? "page[sz]" : "maxPage[sz]") << "=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_HORCENTER:
  case StarAttribute::ATTR_SC_PAGE_VERCENTER:
  case StarAttribute::ATTR_SC_PAGE_ON:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_HORCENTER ? "scPageHor[center]" : nWhich==StarAttribute::ATTR_SC_PAGE_VERCENTER ? "scPageVer[center]" : "pageOn")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_DYNAMIC:
  case StarAttribute::ATTR_SC_PAGE_SHARED:
  case StarAttribute::ATTR_SC_PAGE_NOTES:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_DYNAMIC ? "page[dynamic]" : nWhich==StarAttribute::ATTR_SC_PAGE_SHARED ? "page[shared]" : "page[note]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_GRID:
  case StarAttribute::ATTR_SC_PAGE_HEADERS:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_GRID ? "page[grid]" : "page[headers]") << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_CHARTS:
  case StarAttribute::ATTR_SC_PAGE_OBJECTS:
  case StarAttribute::ATTR_SC_PAGE_DRAWINGS:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_CHARTS ? "page[charts]" : nWhich==StarAttribute::ATTR_SC_PAGE_OBJECTS ? "page[object]" : "page[drawings]");
    if (nVers==0) f << ",";
    else f << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_TOPDOWN:
    f << "scPageTopDown=" << input->readULong(1);
    break;
  case StarAttribute::ATTR_SC_PAGE_SCALE:
  case StarAttribute::ATTR_SC_PAGE_SCALETOPAGES:
  case StarAttribute::ATTR_SC_PAGE_FIRSTPAGENO:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_SCALE ? "page[scale]" : nWhich==StarAttribute::ATTR_SC_PAGE_SCALETOPAGES ? "page[scaleToPage]" : "firstPageNo")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SC_PAGE_PRINTAREA:
  case StarAttribute::ATTR_SC_PAGE_REPEATROW:
  case StarAttribute::ATTR_SC_PAGE_REPEATCOL:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_PRINTAREA ? "print[area]" : nWhich==StarAttribute::ATTR_SC_PAGE_REPEATROW ? "repeat[row]" : "repeat[col]") << ",";
    // sc_attrib.cxx ScRangeItem::Create
    if (nVers==0) {
      uint16_t n, nColS, nRowS, nColE, nRowE;
      *input >> n >> nColS >> nRowS >> nColE >> nRowE;
      if (n>255) f << "allTabs,";
      f << "range=" << nColS << "x" << nRowS << "<->" << nColE << "x" << nRowE << ",";
    }
    else {
      uint16_t nColS, nRowS, nColE, nRowE;
      *input >> nColS >> nRowS >> nColE >> nRowE;
      f << "range=" << nColS << "x" << nRowS << "<->" << nColE << "x" << nRowE << ",";
      if (nVers>=2) {
        bool newFlag;
        *input>>newFlag;
        if (newFlag) f << "newFlag,";
        if (input->tell()+1==lastPos) { // checkme
          *input>>newFlag;
          if (newFlag) f << "newFlag1,";
        }
      }
    }
    break;
  case StarAttribute::ATTR_SC_PAGE_PRINTTABLES: {
    f << "print[tables],";
    uint16_t n;
    *input >> n;
    if (!n||input->tell()+2*int(n)>lastPos) break;
    f << "list=[";
    for (int i=0; i<int(n); ++i)
      f << input->readULong(2) << ",";
    f << "],";
    break;
  }
  case StarAttribute::ATTR_SC_PAGE_HEADERLEFT:
  case StarAttribute::ATTR_SC_PAGE_FOOTERLEFT:
  case StarAttribute::ATTR_SC_PAGE_HEADERRIGHT:
  case StarAttribute::ATTR_SC_PAGE_FOOTERRIGHT: {
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_HEADERLEFT ? "header[left]" :
          nWhich==StarAttribute::ATTR_SC_PAGE_FOOTERLEFT ? "footer[left]" :
          nWhich==StarAttribute::ATTR_SC_PAGE_HEADERRIGHT ? "header[right]" : "footer[right]") << ",";
    for (int i=0; i<3; ++i) {
      long actPos=input->tell();
      StarFileManager fileManager;
      if (!fileManager.readEditTextObject(zone, lastPos, document) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a text object\n"));
        ascFile.addPos(actPos);
        ascFile.addNote("StarAttribute:###editTextObject");
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
        break;
      }
    }
    break;
  }
  case StarAttribute::ATTR_SC_PAGE_HEADERSET:
  case StarAttribute::ATTR_SC_PAGE_FOOTERSET: {
    // svx_pageitem.cxx SvxSetItem::Create
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_HEADERSET ? "page[headerSet]" : "page[footerSet]") << ",";
    // TODO
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: reading a header/footer set item is not implemented\n"));
      first=false;
    }
    f << "##";
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    break;
  }
  case StarAttribute::ATTR_SC_PAGE_FORMULAS:
  case StarAttribute::ATTR_SC_PAGE_NULLVALS:
    f << (nWhich==StarAttribute::ATTR_SC_PAGE_FORMULAS ? "page[formulas]" : "page[numVals") << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_EE_PARA_NUMBULLET: {
    // svx_numitem.cxx SvxNumRule::SvxNumRule
    f << "eeParaNumBullet,";
    uint16_t version, levelC, nFeatureFlags, nContinuousNumb, numberingType;
    *input >> version >> levelC >> nFeatureFlags >> nContinuousNumb >> numberingType;
    if (version) f << "vers=" << version << ",";
    if (levelC) f << "level[count]=" << levelC << ",";
    if (nFeatureFlags) f << "feature[flags]=" << nFeatureFlags << ",";
    if (nContinuousNumb) f << "continuous[numbering],";
    if (numberingType) f << "number[type]=" << numberingType << ",";
    f << "set=[";
    for (int i=0; i<10; ++i) {
      uint16_t nSet;
      *input>>nSet;
      if (nSet) {
        f << nSet << ",";
        SWFormatManager formatManager;
        if (!formatManager.readNumberFormat(zone, lastPos)) {
          f << "###";
          break;
        }
      }
      else
        f << "_";
    }
    f << "],";
    if (nVers>=2) {
      *input>>nFeatureFlags;
      f << "nFeatureFlags2=" << nFeatureFlags << ",";
    }
    break;
  }
  case StarAttribute::ATTR_EE_PARA_BULLETSTATE:
  case StarAttribute::ATTR_EE_PARA_OUTLLEVEL:
    f << (nWhich==StarAttribute::ATTR_EE_PARA_BULLETSTATE ? "eeBulletState" : "eeOutLevel") << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_EE_PARA_BULLET: {
    // svx_bulitem.cxx SvxBulletItem::SvxBulletItem
    f << "paraBullet,";
    uint16_t style;
    *input >> style;
    if (style==128) {
      StarBitmap bitmap;
      if (!bitmap.readBitmap(zone, true, lastPos)) {
        f << "###BM,";
        break;
      }
    }
    else {
      // SvxBulletItem::CreateFont
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      else if (!col.isBlack())
        f << "color=" << col << ",";
      uint16_t family, encoding, pitch, align, weight, underline, strikeOut, italic;
      *input >> family >> encoding >> pitch >> align >> weight >> underline >> strikeOut >> italic;
      if (family) f << "family=" << family << ",";
      if (encoding) f << "encoding=" << encoding << ",";
      if (pitch) f << "pitch=" << pitch << ",";
      if (weight) f << "weight=" << weight << ",";
      if (underline) f << "underline=" << underline << ",";
      if (strikeOut) f << "strikeOut=" << strikeOut << ",";
      if (italic) f << "italic=" << italic << ",";
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a name\n"));
        f << "###text,";
        break;
      }
      f << text.cstr() << ",";
      // if (nVers==1) f << "size=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      bool outline, shadow, transparent;
      *input >> outline >>  shadow >>  transparent;
      if (outline) f << "outline,";
      if (shadow) f << "shadow,";
      if (transparent) f << "transparent,";
    }
    f << "width=" << input->readLong(4) << ",";
    f << "start=" << input->readULong(2) << ",";
    f << "justify=" << input->readULong(1) << ",";
    f << "symbol=" << input->readULong(1) << ",";
    f << "scale=" << input->readULong(2) << ",";
    for (int i=0; i<2; ++i) {
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a name\n"));
        f << "###text,";
        break;
      }
      else if (!text.empty())
        f << (i==0 ? "prev" : "next") << "=" << text.cstr() << ",";
    }
    break;
  }
  case StarAttribute::ATTR_EE_CHR_RUBI_DUMMY: // ok no data
  case StarAttribute::ATTR_EE_FEATURE_TAB:
  case StarAttribute::ATTR_EE_FEATURE_LINEBR:
    f << (nWhich==StarAttribute::ATTR_EE_CHR_RUBI_DUMMY ? "eeRubiDummy" :
          nWhich==StarAttribute::ATTR_EE_FEATURE_TAB ? "eeFeatureTab" : "eeFeatureLineBr") << ",";
    break;
  case StarAttribute::ATTR_EE_FEATURE_FIELD: {
    f << "eeFeatureField,vers=" << nVers << ",";
    // svx_flditem.cxx SvxFieldItem::Create
    if (!document.readPersistData(zone, lastPos)) break;
    return true;
  }

  case StarAttribute::ATTR_SCH_DATADESCR_DESCR:
    f << "data[desc]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SCH_DATADESCR_SHOW_SYM:
    f << "dataDesc[showSym]=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_LEGEND_POS:
  case StarAttribute::ATTR_SCH_TEXT_ORIENT:
  case StarAttribute::ATTR_SCH_TEXT_ORDER:
    f << (nWhich==StarAttribute::ATTR_SCH_LEGEND_POS ? "legend[pos]" :
          nWhich==StarAttribute::ATTR_SCH_TEXT_ORIENT ? "text[orient]" : "text[order]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_AUTO_MIN:
  case StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MIN:
  case StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MIN:
  case StarAttribute::ATTR_SCH_AXIS_AUTO_MIN:
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_MIN ? "xAxisAutoMin" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MIN ? "yAxisAutoMin" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MIN ? "zAxisAutoMin" : "axisAutoMin")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_MIN:
  case StarAttribute::ATTR_SCH_Y_AXIS_MIN:
  case StarAttribute::ATTR_SCH_Z_AXIS_MIN:
  case StarAttribute::ATTR_SCH_AXIS_MIN: {
    double value;
    *input >> value;
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_MIN ? "xAxisMin" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MIN ? "yAxisMin" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MIN ? "zAxisMin" : "axisMin")
      << "=" << value << ",";
    break;
  }
  case StarAttribute::ATTR_SCH_X_AXIS_AUTO_MAX:
  case StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MAX:
  case StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MAX:
  case StarAttribute::ATTR_SCH_AXIS_AUTO_MAX:
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_MAX ? "xAxisAutoMax" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MAX ? "yAxisAutoMax" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MAX ? "zAxisAutoMax" : "axisAutoMax")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_MAX:
  case StarAttribute::ATTR_SCH_Y_AXIS_MAX:
  case StarAttribute::ATTR_SCH_Z_AXIS_MAX:
  case StarAttribute::ATTR_SCH_AXIS_MAX: {
    double value;
    *input >> value;
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_MAX ? "xAxisMax" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MAX ? "yAxisMax" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MAX ? "zAxisMax" : "axisMax")
      << "=" << value << ",";
    break;
  }
  case StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_MAIN:
  case StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN:
  case StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN:
  case StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_MAIN:
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_MAIN ? "xAxisAutoStepMain" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN ? "yAxisAutoStepMain" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN ? "zAxisAutoStepMain" : "axisAutoStepMain")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_STEP_MAIN:
  case StarAttribute::ATTR_SCH_Y_AXIS_STEP_MAIN:
  case StarAttribute::ATTR_SCH_Z_AXIS_STEP_MAIN:
  case StarAttribute::ATTR_SCH_AXIS_STEP_MAIN: {
    double value;
    *input >> value;
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_STEP_MAIN ? "xAxisStepMain" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_STEP_MAIN ? "yAxisStepMain" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_STEP_MAIN ? "zAxisStepMain" : "axisStepMain")
      << "=" << value << ",";
    break;
  }
  case StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_HELP:
  case StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_HELP:
  case StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_HELP:
  case StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_HELP:
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_HELP ? "xAxisAutoStepHelp" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_HELP ? "yAxisAutoStepHelp" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_HELP ? "zAxisAutoStepHelp" : "axisAutoStepHelp")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_STEP_HELP:
  case StarAttribute::ATTR_SCH_Y_AXIS_STEP_HELP:
  case StarAttribute::ATTR_SCH_Z_AXIS_STEP_HELP:
  case StarAttribute::ATTR_SCH_AXIS_STEP_HELP: {
    double value;
    *input >> value;
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_STEP_HELP ? "xAxisStepHelp" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_STEP_HELP ? "yAxisStepHelp" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_STEP_HELP ? "zAxisStepHelp" : "axisStepHelp")
      << "=" << value << ",";
    break;
  }
  case StarAttribute::ATTR_SCH_X_AXIS_LOGARITHM:
  case StarAttribute::ATTR_SCH_Y_AXIS_LOGARITHM:
  case StarAttribute::ATTR_SCH_Z_AXIS_LOGARITHM:
  case StarAttribute::ATTR_SCH_AXIS_LOGARITHM:
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_LOGARITHM ? "xAxisLogarithm" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_LOGARITHM ? "yAxisLogarithm" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_LOGARITHM ? "zAxisLogarithm" : "axisLogarithm")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_AUTO_ORIGIN:
  case StarAttribute::ATTR_SCH_Y_AXIS_AUTO_ORIGIN:
  case StarAttribute::ATTR_SCH_Z_AXIS_AUTO_ORIGIN:
  case StarAttribute::ATTR_SCH_AXIS_AUTO_ORIGIN:
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_ORIGIN ? "xAxisAutoOrigin" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_ORIGIN ? "yAxisAutoOrigin" :
          nWhich==StarAttribute::ATTR_SCH_Z_AXIS_AUTO_ORIGIN ? "zAxisAutoOrigin" : "axisAutoOrigin")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_X_AXIS_ORIGIN:
  case StarAttribute::ATTR_SCH_Y_AXIS_ORIGIN:
  case StarAttribute::ATTR_SCH_Z_AXIS_ORIGIN:
  case StarAttribute::ATTR_SCH_AXIS_ORIGIN: {
    double value;
    *input >> value;
    f << (nWhich==StarAttribute::ATTR_SCH_X_AXIS_AUTO_ORIGIN ? "xAxisOrigin" :
          nWhich==StarAttribute::ATTR_SCH_Y_AXIS_AUTO_ORIGIN ? "yAxisOrigin" :
          nWhich==StarAttribute::ATTR_SCH_AXIS_AUTO_ORIGIN ? "zAxisOrigin" : "axisOrigin")
      << "=" << value << ",";
    break;
  }
  case StarAttribute::ATTR_SCH_AXISTYPE:
  case StarAttribute::ATTR_SCH_DUMMY0:
  case StarAttribute::ATTR_SCH_DUMMY1:
    f << (nWhich==StarAttribute::ATTR_SCH_AXISTYPE ? "axis[type]" : nWhich==StarAttribute::ATTR_SCH_DUMMY0 ? "dummy0[sch]" : "dummy1[sch]")
      << "=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_DUMMY2:
  case StarAttribute::ATTR_SCH_DUMMY3:
  case StarAttribute::ATTR_SCH_DUMMY_END:
    f << (nWhich==StarAttribute::ATTR_SCH_DUMMY2 ? "dummy2[sch]" : nWhich==StarAttribute::ATTR_SCH_DUMMY3 ? "dummy3[sch]" : "dummyEnd[sch]")
      << "=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_STAT_AVERAGE:
    f << "statAverage=" << input->readULong(1) << ",";
    break;

  case StarAttribute::ATTR_SCH_STAT_KIND_ERROR:
    f << "statKindError=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_STAT_PERCENT:
  case StarAttribute::ATTR_SCH_STAT_BIGERROR:
  case StarAttribute::ATTR_SCH_STAT_CONSTPLUS:
  case StarAttribute::ATTR_SCH_STAT_CONSTMINUS: {
    double value;
    *input>>value;
    f << (nWhich==StarAttribute::ATTR_SCH_STAT_PERCENT ? "stat[percent]" : nWhich==StarAttribute::ATTR_SCH_STAT_BIGERROR ? "stat[bigError]" :
          nWhich==StarAttribute::ATTR_SCH_STAT_CONSTPLUS ? "stat[constPlus]" : "stat[constMinus]")
      << "=" << value << ",";
    break;
  }

  case StarAttribute::ATTR_SCH_STAT_REGRESSTYPE:
  case StarAttribute::ATTR_SCH_STAT_INDICATE:
  case StarAttribute::ATTR_SCH_TEXT_DEGREES:
    f << (nWhich==StarAttribute::ATTR_SCH_STAT_REGRESSTYPE ? "stat[regType]" :
          nWhich==StarAttribute::ATTR_SCH_STAT_INDICATE ? "stat[indicate]" : "stat[textDegree")
      << "=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_TEXT_OVERLAP:
    f << "overlap[sch,text]=" << input->readULong(1) << ",";
    break;

  case StarAttribute::ATTR_SCH_TEXT_DUMMY0:
  case StarAttribute::ATTR_SCH_TEXT_DUMMY1:
  case StarAttribute::ATTR_SCH_TEXT_DUMMY2:
  case StarAttribute::ATTR_SCH_TEXT_DUMMY3:
    f << (nWhich==StarAttribute::ATTR_SCH_TEXT_DUMMY0 ? "dummy0[sch,text]" :
          nWhich==StarAttribute::ATTR_SCH_TEXT_DUMMY1 ? "dummy1[sch,text]" :
          nWhich==StarAttribute::ATTR_SCH_TEXT_DUMMY2 ? "dummy2[sch,text]" : "dummy3[sch,text]")
      << "=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_STYLE_DEEP:
  case StarAttribute::ATTR_SCH_STYLE_3D:
  case StarAttribute::ATTR_SCH_STYLE_VERTICAL:
    f << (nWhich==StarAttribute::ATTR_SCH_STYLE_DEEP ? "deep[style]" : nWhich==StarAttribute::ATTR_SCH_STYLE_3D ? "3d[style]" : "vert[style]")
      << "=" << input->readULong(1) << ",";
    break;

  case StarAttribute::ATTR_SCH_STYLE_BASETYPE:
    f << "baseType[style]=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_STYLE_LINES:
  case StarAttribute::ATTR_SCH_STYLE_PERCENT:
  case StarAttribute::ATTR_SCH_STYLE_STACKED:
    f << (nWhich==StarAttribute::ATTR_SCH_STYLE_LINES ? "lines[style]" :
          nWhich==StarAttribute::ATTR_SCH_STYLE_PERCENT ? "percent[style]" : "stacked[style]")
      << "=" << input->readULong(1) << ",";
    break;

  case StarAttribute::ATTR_SCH_STYLE_SPLINES:
  case StarAttribute::ATTR_SCH_STYLE_SYMBOL:
  case StarAttribute::ATTR_SCH_STYLE_SHAPE:
    f << (nWhich==StarAttribute::ATTR_SCH_STYLE_SPLINES ? "splines[style]" :
          nWhich==StarAttribute::ATTR_SCH_STYLE_SYMBOL ? "symbol[style]" : "shape[style]")
      << "=" << input->readLong(4);
    break;

  case StarAttribute::ATTR_SCH_AXIS:
    f << "axis[sch]=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_SCH_AXIS_HELPTICKS:
  case StarAttribute::ATTR_SCH_AXIS_TICKS:
  case StarAttribute::ATTR_SCH_AXIS_NUMFMT:
  case StarAttribute::ATTR_SCH_AXIS_NUMFMTPERCENT:
    f << (nWhich==StarAttribute::ATTR_SCH_AXIS_HELPTICKS ? "axis[helpticks]" :
          nWhich==StarAttribute::ATTR_SCH_AXIS_TICKS ? "axis[ticks]" :
          nWhich==StarAttribute::ATTR_SCH_AXIS_NUMFMT ? "axis[numfmt]" : "axis[numfmtpercent]")
      << "=" << input->readLong(4);
    break;

  case StarAttribute::ATTR_SCH_AXIS_SHOWAXIS:
  case StarAttribute::ATTR_SCH_AXIS_SHOWDESCR:
  case StarAttribute::ATTR_SCH_AXIS_SHOWMAINGRID:
    f << (nWhich==StarAttribute::ATTR_SCH_AXIS_SHOWAXIS ? "show[axis]" :
          nWhich==StarAttribute::ATTR_SCH_AXIS_SHOWDESCR ? "show[descr]" : "show[maingrid]")
      << "=" << input->readULong(1) << ",";
    break;

  case StarAttribute::ATTR_SCH_AXIS_SHOWHELPGRID:
  case StarAttribute::ATTR_SCH_AXIS_TOPDOWN:
    f << (nWhich==StarAttribute::ATTR_SCH_AXIS_SHOWHELPGRID ? "show[helpGrid]" : "topDown")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_AXIS_DUMMY0:
  case StarAttribute::ATTR_SCH_AXIS_DUMMY1:
  case StarAttribute::ATTR_SCH_AXIS_DUMMY2:
    f << (nWhich==StarAttribute::ATTR_SCH_AXIS_DUMMY0 ? "dummy0[sch,axis]" :
          nWhich==StarAttribute::ATTR_SCH_AXIS_DUMMY1 ? "dummy1[sch,axis]" : "dummy2[sch,axis]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::ATTR_SCH_AXIS_DUMMY3:
  case StarAttribute::ATTR_SCH_BAR_OVERLAP:
  case StarAttribute::ATTR_SCH_BAR_GAPWIDTH:
    f << (nWhich==StarAttribute::ATTR_SCH_AXIS_DUMMY3 ? "dummy3[sch,axis]" :
          nWhich==StarAttribute::ATTR_SCH_BAR_OVERLAP ? "bar[overlap]" : "bar[gapwidth]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::ATTR_SCH_STOCK_VOLUME:
  case StarAttribute::ATTR_SCH_STOCK_UPDOWN:
    f << (nWhich==StarAttribute::ATTR_SCH_STOCK_VOLUME ? "stock[volume]" : "stock[updown]") << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_SCH_SYMBOL_SIZE:
    f << "symbolSize[sch]=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    break;

  case StarAttribute::XATTR_LINESTYLE:
  case StarAttribute::XATTR_FILLSTYLE:
    f << (nWhich==StarAttribute::XATTR_LINESTYLE ? "line[style]" : "fill[style]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::XATTR_LINEWIDTH:
  case StarAttribute::XATTR_LINESTARTWIDTH:
  case StarAttribute::XATTR_LINEENDWIDTH:
    f << (nWhich==StarAttribute::XATTR_LINEWIDTH ? "line[width]" :
          nWhich==StarAttribute::XATTR_LINESTARTWIDTH ? "line[startWidth]" : "line[endWidth]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::XATTR_LINESTARTCENTER:
  case StarAttribute::XATTR_LINEENDCENTER:
    f << (nWhich==StarAttribute::XATTR_LINESTARTCENTER ? "line[startCenter]" : "line[endCenter]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::XATTR_LINETRANSPARENCE:
    f << "lineTransparence=" << input->readULong(2) << ",";
    break;
  case StarAttribute::XATTR_LINEJOINT:
    f << "lineJoint=" << input->readULong(2) << ",";
    break;

  // name or index
  case StarAttribute::XATTR_LINEDASH:
  case StarAttribute::XATTR_LINECOLOR:
  case StarAttribute::XATTR_LINESTART:
  case StarAttribute::XATTR_LINEEND:
  case StarAttribute::XATTR_FILLCOLOR:
  case StarAttribute::XATTR_FILLGRADIENT:
  case StarAttribute::XATTR_FILLHATCH:
  case StarAttribute::XATTR_FILLBITMAP:
  case StarAttribute::XATTR_FILLFLOATTRANSPARENCE:

  case StarAttribute::XATTR_FORMTXTSHDWCOLOR: {
    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    if (!text.empty()) f << "name=" << text.cstr() << ",";
    int32_t id;
    *input >> id;
    if (id>=0) f << "id=" << id << ",";
    switch (nWhich) {
    case StarAttribute::XATTR_LINEDASH: {
      f << "line[dash],";
      if (id>=0) break;
      int32_t dashStyle;
      uint16_t dots, dashes;
      uint32_t dotLen, dashLen, distance;
      *input >> dashStyle >> dots >> dotLen >> dashes >> dashLen >> distance;
      if (dashStyle) f << "style=" << dashStyle << ",";
      if (dots || dotLen) f << "dots=" << dots << ":" << dotLen << ",";
      if (dashes || dashLen) f << "dashs=" << dashes << ":" << dashLen << ",";
      if (distance) f << "distance=" << distance << ",";
      break;
    }
    case StarAttribute::XATTR_LINECOLOR:
    case StarAttribute::XATTR_FILLCOLOR:
    case StarAttribute::XATTR_FORMTXTSHDWCOLOR: {
      f << (nWhich==StarAttribute::XATTR_LINECOLOR ? "line[color]" :
            nWhich==StarAttribute::XATTR_FILLCOLOR ? "fill[color]" : "shadow[form,color]") << ",";
      if (id>=0) break;
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      if (!col.isBlack()) f << col << ",";
      break;
    }
    case StarAttribute::XATTR_LINESTART:
    case StarAttribute::XATTR_LINEEND: {
      f << (nWhich==StarAttribute::XATTR_LINESTART ? "line[start]" : "line[end]") << ",";
      if (id>=0) break;
      uint32_t nPoints;
      *input >> nPoints;
      if (input->tell()+6*long(nPoints)>lastPos) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: bad num point\n"));
        f << "###nPoints=" << nPoints << ",";
        break;
      }
      f << "pts=[";
      for (uint32_t i=0; i<nPoints; ++i)
        f << input->readLong(4) << "x" << input->readLong(4) << ":" << input->readULong(4) << ",";
      f << "],";
      break;
    }
    case StarAttribute::XATTR_FILLGRADIENT:
    case StarAttribute::XATTR_FILLFLOATTRANSPARENCE: {
      f << (nWhich==StarAttribute::XATTR_FILLGRADIENT ? "gradient[fill]" : "transparence[float,fill]")  << ",";
      if (id>=0) break;
      uint16_t type, red, green, blue, red2, green2, blue2;
      *input >> type >> red >> green >> blue >> red2 >> green2 >> blue2;
      if (type) f << "type=" << type << ",";
      f << "startColor=" << STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8)) << ",";
      f << "endColor=" << STOFFColor(uint8_t(red2>>8),uint8_t(green2>>8),uint8_t(blue2>>8)) << ",";
      uint32_t angle;
      uint16_t border, xOffset, yOffset, startIntens, endIntens, nStep;
      *input >> angle >> border >> xOffset >> yOffset >> startIntens >> endIntens;
      if (angle) f << "angle=" << angle << ",";
      if (border) f << "border=" << border << ",";
      f << "offset=" << xOffset << "x" << yOffset << ",";
      f << "intensity=[" << startIntens << "," << endIntens << "],";
      if (nVers>=1) {
        *input >> nStep;
        if (nStep) f << "step=" << nStep << ",";
      }
      if (nWhich==StarAttribute::XATTR_FILLFLOATTRANSPARENCE) {
        bool enabled;
        *input>>enabled;
        if (enabled) f << "enabled,";
      }
      break;
    }
    case StarAttribute::XATTR_FILLHATCH: {
      f << "hatch[fill],";
      if (id>=0) break;
      uint16_t type, red, green, blue;
      int32_t distance, angle;
      *input >> type >> red >> green >> blue >> distance >> angle;
      if (type) f << "type=" << type << ",";
      f << "color=" << STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8)) << ",";
      f << "distance=" << distance << ",";
      f << "angle=" << angle << ",";
      break;
    }
    case StarAttribute::XATTR_FILLBITMAP: {
      f << "bitmap[fill],";
      if (id>=0) break;
      if (nVers==1) {
        int16_t style, type;
        *input >> style >> type;
        if (style) f << "style=" << style << ",";
        if (type) f << "type=" << type << ",";
        if (type==1) {
          if (input->tell()+128+2>lastPos) {
            STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: the zone seems too short\n"));
            f << "###short,";
            break;
          }
          f << "val=[";
          for (int i=0; i<64; ++i) {
            uint16_t value;
            *input>>value;
            if (value) f << std::hex << value << std::dec << ",";
            else f << "_,";
          }
          f << "],";
          for (int i=0; i<2; ++i) {
            STOFFColor col;
            if (!input->readColor(col)) {
              STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
              f << "###color,";
              break;
            }
            f << "col" << i << "=" << col << ",";
          }
          break;
        }
        if (type==2) break;
        if (type!=0) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: unexpected type\n"));
          f << "###type,";
          break;
        }
      }
      StarBitmap bitmap;
      if (!bitmap.readBitmap(zone, true, lastPos)) break;
      break;
    }

    default:
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: unknown name id\n"));
      f << "###nameId,";
      break;
    }
    break;
  }

  case StarAttribute::XATTR_SET_LINE:
    f << "setLine,";
    document.readItemSet(zone, 1000, 1016, lastPos, document.getCurrentPool().get());
    break;
  case StarAttribute::XATTR_SET_FILL:
  case StarAttribute::XATTR_SET_TEXT: {
    f << (nWhich==StarAttribute::XATTR_SET_FILL ? "setFill" : "setText") << ",";
    StarFileManager fileManager;
    fileManager.readSfxItemList(zone);
    break;
  }
  case StarAttribute::XATTR_FILLTRANSPARENCE:
  case StarAttribute::XATTR_GRADIENTSTEPCOUNT:
  case StarAttribute::XATTR_FILLBMP_POS:
    f << (nWhich==StarAttribute::XATTR_FILLTRANSPARENCE ? "transparence[fill]" :
          nWhich==StarAttribute::XATTR_GRADIENTSTEPCOUNT ? "gradient[stepCount]" : "bmp[pos]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::XATTR_FILLBMP_TILE:
  case StarAttribute::XATTR_FILLBMP_STRETCH:
  case StarAttribute::XATTR_FILLBMP_SIZELOG:
    f << (nWhich==StarAttribute::XATTR_FILLBMP_TILE ? "bmp[title]" :
          nWhich==StarAttribute::XATTR_FILLBMP_STRETCH ? "bmp[strech]" : "bmp[sizelog]") << "=" << input->readULong(1) <<",";
    break;
  case StarAttribute::XATTR_FILLBMP_SIZEX:
  case StarAttribute::XATTR_FILLBMP_SIZEY:
    f << (nWhich==StarAttribute::XATTR_FILLBMP_SIZEX ? "bmp[sizeX]" : "bmp[sizeY]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::XATTR_FILLBMP_TILEOFFSETX:
  case StarAttribute::XATTR_FILLBMP_TILEOFFSETY:
    f << (nWhich==StarAttribute::XATTR_FILLBMP_TILEOFFSETX ? "bmp[offsX]" : "bmp[offsY]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::XATTR_FILLBMP_POSOFFSETX:
  case StarAttribute::XATTR_FILLBMP_POSOFFSETY:
    f << (nWhich==StarAttribute::XATTR_FILLBMP_POSOFFSETX ? "bmp[posOffsX]" : "bmp[posOffsY]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::XATTR_FILLBACKGROUND:
    f << "fill[background]=" << input->readULong(1) << ",";
    break;

  case StarAttribute::XATTR_FORMTXTSTYLE:
  case StarAttribute::XATTR_FORMTXTADJUST:
  case StarAttribute::XATTR_FORMTXTSHADOW:
    f << (nWhich==StarAttribute::XATTR_FORMTXTSTYLE ? "style[form]" :
          nWhich==StarAttribute::XATTR_FORMTXTADJUST ? "adjust[form]" : "shadow[form]")
      << "=" << input->readULong(2) << ",";
    break;

  case StarAttribute::XATTR_FORMTXTDISTANCE:
  case StarAttribute::XATTR_FORMTXTSTART:
    f << (nWhich==StarAttribute::XATTR_FORMTXTDISTANCE ? "distance[form]" : "start[form]") << "=" << input->readLong(4) << ",";
    break;

  case StarAttribute::XATTR_FORMTXTMIRROR:
  case StarAttribute::XATTR_FORMTXTOUTLINE:
  case StarAttribute::XATTR_FORMTXTHIDEFORM:
    f << (nWhich==StarAttribute::XATTR_FORMTXTMIRROR ? "mirror[form]" :
          nWhich==StarAttribute::XATTR_FORMTXTOUTLINE ? "outline[form]" : "hide[form]") << "=" << input->readULong(1) << ",";
    break;

  case StarAttribute::XATTR_FORMTXTSHDWXVAL:
  case StarAttribute::XATTR_FORMTXTSHDWYVAL:
    f << (nWhich==StarAttribute::XATTR_FORMTXTSHDWXVAL ? "shadowX[form]" : "shadowY[form]") << "=" << input->readLong(4) << ",";
    break;

  case StarAttribute::XATTR_FORMTXTSTDFORM:
  case StarAttribute::XATTR_FORMTXTSHDWTRANSP:
    f << (nWhich==StarAttribute::XATTR_FORMTXTSTDFORM ? "standart[form]" : "shadowTrans[form]") << "=" << input->readULong(2) << ",";
    break;

  case StarAttribute::SDRATTR_SET_SHADOW:
  case StarAttribute::SDRATTR_SET_CAPTION:
  case StarAttribute::SDRATTR_SET_OUTLINER:
  case StarAttribute::SDRATTR_SET_MISC:
  case StarAttribute::SDRATTR_SET_EDGE:
  case StarAttribute::SDRATTR_SET_MEASURE:
  case StarAttribute::SDRATTR_SET_CIRC:
  case StarAttribute::SDRATTR_SET_GRAF: {
    f << (nWhich==StarAttribute::SDRATTR_SET_SHADOW ? "setSdrShadow" :
          nWhich==StarAttribute::SDRATTR_SET_CAPTION ? "setSdrCaption" :
          nWhich==StarAttribute::SDRATTR_SET_OUTLINER ? "setSdrOutliner" :
          nWhich==StarAttribute::SDRATTR_SET_MISC ? "setSdrMisc" :
          nWhich==StarAttribute::SDRATTR_SET_EDGE ? "setSdrEdge" :
          nWhich==StarAttribute::SDRATTR_SET_MEASURE ? "setSdrMeasure" :
          nWhich==StarAttribute::SDRATTR_SET_CIRC ? "setCircle" : "setGraph") << ",";
    StarFileManager fileManager;
    fileManager.readSfxItemList(zone);
    break;
  }

  case StarAttribute::SDRATTR_SHADOW3D:
  case StarAttribute::SDRATTR_SHADOWPERSP:
    f << (nWhich==StarAttribute::SDRATTR_SHADOW3D ? "sdrShadow3d" : "sdrShadowPersp") << ",";
    break;

  case StarAttribute::SDRATTR_SHADOW:
    f << "sdrShadow=" << input->readULong(1) << ",";
    break;

  // name or index
  case StarAttribute::SDRATTR_SHADOWCOLOR: {
    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    if (!text.empty()) f << "name=" << text.cstr() << ",";
    int32_t id;
    *input >> id;
    if (id>=0) f << "id=" << id << ",";
    switch (nWhich) {
    case StarAttribute::SDRATTR_SHADOWCOLOR: {
      f << "sdrShadow[color],";
      if (id>=0) break;
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
        f << "###color,";
        break;
      }
      if (!col.isBlack()) f << col << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: unknown name id\n"));
      f << "###nameId,";
      break;
    }
    break;
  }
  case StarAttribute::SDRATTR_SHADOWXDIST:
  case StarAttribute::SDRATTR_SHADOWYDIST:
    f << (nWhich==StarAttribute::SDRATTR_SHADOWXDIST ? "sdrXDist[shadow]" : "sdrYDist[shadow]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_SHADOWTRANSPARENCE:
    f << "sdrTransparence[shadow]=" << input->readULong(2) << ",";
    break;

  case StarAttribute::SDRATTR_SHADOWRESERVE1:
  case StarAttribute::SDRATTR_SHADOWRESERVE2:
  case StarAttribute::SDRATTR_SHADOWRESERVE3:
  case StarAttribute::SDRATTR_SHADOWRESERVE4:
  case StarAttribute::SDRATTR_SHADOWRESERVE5:
    f << "shadowReserved" << nWhich-StarAttribute::SDRATTR_SHADOWRESERVE1+1 << "[sdr],";
    break;

  case StarAttribute::SDRATTR_CAPTIONTYPE:
  case StarAttribute::SDRATTR_CAPTIONESCDIR:
    f << (nWhich==StarAttribute::SDRATTR_CAPTIONTYPE ? "sdrCaption[type]" : "sdrCaption[escDir]") << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_CAPTIONFIXEDANGLE:
  case StarAttribute::SDRATTR_CAPTIONESCISREL:
  case StarAttribute::SDRATTR_CAPTIONFITLINELEN:
    f << (nWhich==StarAttribute::SDRATTR_CAPTIONFIXEDANGLE ? "sdrCaption[fixedAngle]" :
          nWhich==StarAttribute::SDRATTR_CAPTIONESCISREL ? "sdrCaption[escIsRel]" : "sdrCaption[fitLineLen]") << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_CAPTIONANGLE: // angle
    f << "sdrCaption[angle]=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_CAPTIONGAP: // metric
  case StarAttribute::SDRATTR_CAPTIONESCABS:
  case StarAttribute::SDRATTR_CAPTIONLINELEN:
    f << (nWhich==StarAttribute::SDRATTR_CAPTIONGAP ? "sdrCaption[gap]" :
          nWhich==StarAttribute::SDRATTR_CAPTIONESCABS ? "sdrCaption[escAbs]" : "sdrCaption[lineLen]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_CAPTIONESCREL:
    f << "sdrCaption[escRel]=" << input->readLong(4) << ",";
    break;

  case StarAttribute::SDRATTR_CAPTIONRESERVE1:
  case StarAttribute::SDRATTR_CAPTIONRESERVE2:
  case StarAttribute::SDRATTR_CAPTIONRESERVE3:
  case StarAttribute::SDRATTR_CAPTIONRESERVE4:
  case StarAttribute::SDRATTR_CAPTIONRESERVE5:
    f << "captionReserved" << nWhich-StarAttribute::SDRATTR_CAPTIONRESERVE1+1 << "[sdr},";
    break;

  case StarAttribute::SDRATTR_ECKENRADIUS: // metric
  case StarAttribute::SDRATTR_TEXT_MINFRAMEHEIGHT:
  case StarAttribute::SDRATTR_TEXT_LEFTDIST:
    f << (nWhich==StarAttribute::SDRATTR_ECKENRADIUS ? "eckenRadius[sdr]":
          nWhich==StarAttribute::SDRATTR_TEXT_MINFRAMEHEIGHT ? "textMinHeight[sdr]" : "textLeftDist[sdr]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_AUTOGROWHEIGHT: // onoff
  case StarAttribute::SDRATTR_TEXT_AUTOGROWWIDTH:
    f << (nWhich==StarAttribute::SDRATTR_TEXT_AUTOGROWHEIGHT ? "textAutoGrowHeight[sdr]" : "textAutoGrowWidth[sdr]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_FITTOSIZE:
  case StarAttribute::SDRATTR_TEXT_VERTADJUST:
  case StarAttribute::SDRATTR_TEXT_HORZADJUST:
    f << (nWhich==StarAttribute::SDRATTR_TEXT_FITTOSIZE ? "fitToSize[sdr]" :
          nWhich==StarAttribute::SDRATTR_TEXT_VERTADJUST ? "vertAdjust[sdr]" : "horAdjust[sdr]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_RIGHTDIST: // metric
  case StarAttribute::SDRATTR_TEXT_UPPERDIST:
  case StarAttribute::SDRATTR_TEXT_LOWERDIST:
    f << (nWhich==StarAttribute::SDRATTR_TEXT_RIGHTDIST ? "textRightDist[sdr]" :
          nWhich==StarAttribute::SDRATTR_TEXT_UPPERDIST ? "textUpperDist[sdr]" : "textLowerDist[sdr]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_MAXFRAMEHEIGHT: // metric
  case StarAttribute::SDRATTR_TEXT_MINFRAMEWIDTH:
  case StarAttribute::SDRATTR_TEXT_MAXFRAMEWIDTH:
    f << (nWhich==StarAttribute::SDRATTR_TEXT_MAXFRAMEHEIGHT ? "maxFrmHeight[sdr]" :
          nWhich==StarAttribute::SDRATTR_TEXT_MINFRAMEWIDTH ? "minFrmWidth[sdr]" : "maxFrmWidth[sdr]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_ANIKIND:
  case StarAttribute::SDRATTR_TEXT_ANIDIRECTION:
    f << (nWhich==StarAttribute::SDRATTR_TEXT_ANIKIND ? "aniKind[sdr]" : "aniDir[sdr]") << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_ANISTARTINSIDE: // yesNo
  case StarAttribute::SDRATTR_TEXT_ANISTOPINSIDE:
  case StarAttribute::SDRATTR_TEXT_CONTOURFRAME: // onOff
    f << (nWhich==StarAttribute::SDRATTR_TEXT_ANISTARTINSIDE ? "aniStartInside" :
          nWhich==StarAttribute::SDRATTR_TEXT_ANISTOPINSIDE ? "aniStopInside" : "textContourFrm[sdr]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_ANICOUNT:
  case StarAttribute::SDRATTR_TEXT_ANIDELAY:
    f << (nWhich==StarAttribute::SDRATTR_TEXT_ANICOUNT ? "aniCount" : "aniDelay") << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_TEXT_ANIAMOUNT:
    f << "aniAmount=" << input->readLong(2) << ",";
    break;
  case StarAttribute::SDRATTR_AUTOSHAPE_ADJUSTMENT:
    f << "autoShapeAdjust[sdr],";
    if (nVers) {
      uint32_t n;
      *input >> n;
      if (n) f << "nCount=" << n << ",";
    }
    break;
  case StarAttribute::SDRATTR_XMLATTRIBUTES:
    f << "sdrXmlAttributes,";
    break;
  case StarAttribute::SDRATTR_RESERVE15:
  case StarAttribute::SDRATTR_RESERVE16:
  case StarAttribute::SDRATTR_RESERVE17:
  case StarAttribute::SDRATTR_RESERVE18:
  case StarAttribute::SDRATTR_RESERVE19:
    f << "sdrReserved" << (nWhich-StarAttribute::SDRATTR_RESERVE15+15) << ",";
    break;

  case StarAttribute::SDRATTR_EDGEKIND:
    f << "edge[kind]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_EDGENODE1HORZDIST: // metric
  case StarAttribute::SDRATTR_EDGENODE1VERTDIST:
  case StarAttribute::SDRATTR_EDGENODE2HORZDIST:
    f << (nWhich==StarAttribute::SDRATTR_EDGENODE1HORZDIST ? "edgeNode1HorDist" :
          nWhich==StarAttribute::SDRATTR_EDGENODE1VERTDIST ? "edgeNode1VerDist" : "edgeNode2HorDist")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_EDGENODE2VERTDIST: // metric
  case StarAttribute::SDRATTR_EDGENODE1GLUEDIST:
  case StarAttribute::SDRATTR_EDGENODE2GLUEDIST:
    f << (nWhich==StarAttribute::SDRATTR_EDGENODE2VERTDIST ? "edgeNode1VerDist" :
          nWhich==StarAttribute::SDRATTR_EDGENODE1GLUEDIST ? "edgeNode1GlueDist" : "edgeNode2GlueDist")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_EDGELINEDELTAANZ:
    f << "edge[lineDeltaAnz=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_EDGELINE1DELTA: // metric
  case StarAttribute::SDRATTR_EDGELINE2DELTA:
  case StarAttribute::SDRATTR_EDGELINE3DELTA:
    f << (nWhich==StarAttribute::SDRATTR_EDGELINE1DELTA ? "edgeLine1Delta" :
          nWhich==StarAttribute::SDRATTR_EDGELINE2DELTA ? "edgeLine2Delta" : "edgeLine3Delta")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_EDGERESERVE02:
  case StarAttribute::SDRATTR_EDGERESERVE03:
  case StarAttribute::SDRATTR_EDGERESERVE04:
  case StarAttribute::SDRATTR_EDGERESERVE05:
  case StarAttribute::SDRATTR_EDGERESERVE06:
  case StarAttribute::SDRATTR_EDGERESERVE07:
  case StarAttribute::SDRATTR_EDGERESERVE08:
  case StarAttribute::SDRATTR_EDGERESERVE09:
    f << "sdrEdgeReserve" << nWhich-StarAttribute::SDRATTR_EDGERESERVE02+2 << ",";
    break;

  case StarAttribute::SDRATTR_MEASUREKIND:
  case StarAttribute::SDRATTR_MEASURETEXTHPOS:
  case StarAttribute::SDRATTR_MEASURETEXTVPOS:
    f << (nWhich==StarAttribute::SDRATTR_MEASUREKIND ? "measure[kind]" :
          nWhich==StarAttribute::SDRATTR_MEASURETEXTHPOS ? "measure[extHPos]" : "measure[extVPos]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_MEASURELINEDIST: // metric
  case StarAttribute::SDRATTR_MEASUREHELPLINEOVERHANG:
  case StarAttribute::SDRATTR_MEASUREHELPLINEDIST:
    f << (nWhich==StarAttribute::SDRATTR_MEASURELINEDIST ? "measure[lineDist]" :
          nWhich==StarAttribute::SDRATTR_MEASUREHELPLINEOVERHANG ? "measure[helpLineOverHang]" : "measure[helpLineDist]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREHELPLINE1LEN: // metric
  case StarAttribute::SDRATTR_MEASUREHELPLINE2LEN:
    f << (nWhich==StarAttribute::SDRATTR_MEASUREHELPLINE1LEN ? "measure[helpLine1Len]" : "measure[helpLine2Len]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREBELOWREFEDGE: // yesNo
  case StarAttribute::SDRATTR_MEASURETEXTROTA90:
  case StarAttribute::SDRATTR_MEASURETEXTUPSIDEDOWN:
    f << (nWhich==StarAttribute::SDRATTR_MEASUREBELOWREFEDGE ? "measure[belowRefEdge]" :
          nWhich==StarAttribute::SDRATTR_MEASURETEXTROTA90 ? "measure[textRot90]" : "measure[textUpsideDown]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREOVERHANG: // metric
    f << "measure[overHang]=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREUNIT: // enum
    f << "measure[unit]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_MEASURESCALE: //  // sdrFraction
    f << "measure[scale],mult=" << input->readLong(4) << ",div=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_MEASURESHOWUNIT: // yesNo
  case StarAttribute::SDRATTR_MEASURETEXTAUTOANGLE:
  case StarAttribute::SDRATTR_MEASURETEXTISFIXEDANGLE:
    f << (nWhich==StarAttribute::SDRATTR_MEASURESHOWUNIT ? "measure[showUnit]" :
          nWhich==StarAttribute::SDRATTR_MEASURETEXTAUTOANGLE ? "measure[autoAngle]" : "measure[fixAngle]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREFORMATSTRING: { // string
    f << "measure[formatString]=";
    librevenge::RVNGString format;
    if (!zone.readString(format)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    f << format.cstr() << ",";
    break;
  }
  case StarAttribute::SDRATTR_MEASURETEXTAUTOANGLEVIEW: // sdrAngle
  case StarAttribute::SDRATTR_MEASURETEXTFIXEDANGLE:
    f << (nWhich==StarAttribute::SDRATTR_MEASURETEXTAUTOANGLEVIEW ? "measure[autoAngle]" : "measure[fixedAngle]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_MEASUREDECIMALPLACES: // int16
    f << "measure[decimalPlaces]=" << input->readLong(2) << ",";
    break;
  case StarAttribute::SDRATTR_MEASURERESERVE05:
  case StarAttribute::SDRATTR_MEASURERESERVE06:
  case StarAttribute::SDRATTR_MEASURERESERVE07:
    f << "sdrMeasureReserved" << nWhich-StarAttribute::SDRATTR_MEASURERESERVE05+5 << ",";
    break;

  case StarAttribute::SDRATTR_CIRCKIND: // enum
    f << "sdrCircle[kind]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_CIRCSTARTANGLE: // sdrAngle
  case StarAttribute::SDRATTR_CIRCENDANGLE:
    f << (nWhich==StarAttribute::SDRATTR_CIRCSTARTANGLE ? "circle[start]" : "circle[end]") << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_CIRCRESERVE0:
  case StarAttribute::SDRATTR_CIRCRESERVE1:
  case StarAttribute::SDRATTR_CIRCRESERVE2:
  case StarAttribute::SDRATTR_CIRCRESERVE3:
    f << "sdrCirReserve" << nWhich-StarAttribute::SDRATTR_CIRCRESERVE0 << ",";
    break;

  case StarAttribute::SDRATTR_OBJMOVEPROTECT: // yesNo
  case StarAttribute::SDRATTR_OBJSIZEPROTECT:
  case StarAttribute::SDRATTR_OBJPRINTABLE:
    f << (nWhich==StarAttribute::SDRATTR_OBJMOVEPROTECT ? "sdrObjMoveProtect" :
          nWhich==StarAttribute::SDRATTR_OBJSIZEPROTECT ? "sdrObjSizeProtect" : "sdrObjPrintable")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_LAYERID: // uint16
    f << "sdrLayerId=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_LAYERNAME:
  case StarAttribute::SDRATTR_OBJECTNAME: {
    f << (nWhich==StarAttribute::SDRATTR_LAYERNAME ? "sdrLayerName" : "sdrObjectName") << ",";
    librevenge::RVNGString name;
    if (!zone.readString(name)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    f << name.cstr() << ",";
    break;
  }
  case StarAttribute::SDRATTR_ALLPOSITIONX: // metric
  case StarAttribute::SDRATTR_ALLPOSITIONY:
  case StarAttribute::SDRATTR_ALLSIZEWIDTH:
    f << (nWhich==StarAttribute::SDRATTR_ALLPOSITIONX ? "sdrAllPosX" :
          nWhich==StarAttribute::SDRATTR_ALLPOSITIONY ? "sdrAllPosY" : "allSizeWidth")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_ALLSIZEHEIGHT: // metric
  case StarAttribute::SDRATTR_ONEPOSITIONX:
  case StarAttribute::SDRATTR_ONEPOSITIONY:
    f << (nWhich==StarAttribute::SDRATTR_ALLSIZEHEIGHT ? "sdrAllSizeHeight" :
          nWhich==StarAttribute::SDRATTR_ONEPOSITIONX ? "sdrOnePosX" : "sdrOnePoxY")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_ONESIZEWIDTH: // metric
  case StarAttribute::SDRATTR_ONESIZEHEIGHT:
  case StarAttribute::SDRATTR_LOGICSIZEWIDTH:
    f << (nWhich==StarAttribute::SDRATTR_ONESIZEWIDTH ? "sdrOneSizeWidth" :
          nWhich==StarAttribute::SDRATTR_ONESIZEHEIGHT ? "sdrOneSizeHeight" : "sdrLogicSizeWidth")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_LOGICSIZEHEIGHT: // metric
  case StarAttribute::SDRATTR_MOVEX:
  case StarAttribute::SDRATTR_MOVEY:
    f << (nWhich==StarAttribute::SDRATTR_LOGICSIZEHEIGHT ? "sdrLogicSizeHeight" :
          nWhich==StarAttribute::SDRATTR_MOVEX ? "sdrMoveX" : "sdrMoveY")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_ROTATEANGLE: // sdrAngle
  case StarAttribute::SDRATTR_SHEARANGLE:
    f << (nWhich==StarAttribute::SDRATTR_ROTATEANGLE ? "sdrAngle[rotate]" : "sdrAngle[shear]") << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_RESIZEXONE: // sdrFraction
  case StarAttribute::SDRATTR_RESIZEYONE:
    f << (nWhich==StarAttribute::SDRATTR_RESIZEXONE ? "sdrResizeX[one]" : "sdrResizeY[one]") << ","
      << "mult=" << input->readLong(4) << ",div=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_ROTATEONE: // sdrAngle
  case StarAttribute::SDRATTR_HORZSHEARONE:
  case StarAttribute::SDRATTR_VERTSHEARONE:
    f << (nWhich==StarAttribute::SDRATTR_ROTATEONE ? "sdr[rotateOne]" :
          nWhich==StarAttribute::SDRATTR_HORZSHEARONE ? "sdr[horzShearOne]" : "sdr[vertShearOne]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_RESIZEXALL: // sdrFraction
  case StarAttribute::SDRATTR_RESIZEYALL:
    f << (nWhich==StarAttribute::SDRATTR_RESIZEXALL ? "sdrResizeX[all]" : "sdrResizeY[all]") << ","
      << "mult=" << input->readLong(4) << ",div=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_ROTATEALL: // sdrAngle
  case StarAttribute::SDRATTR_HORZSHEARALL:
  case StarAttribute::SDRATTR_VERTSHEARALL:
    f << (nWhich==StarAttribute::SDRATTR_ROTATEALL ? "sdr[rotateAll]" :
          nWhich==StarAttribute::SDRATTR_HORZSHEARALL ? "sdr[horzShearAll]" : "sdr[vertShearAll]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_TRANSFORMREF1X: // metric
  case StarAttribute::SDRATTR_TRANSFORMREF1Y:
    f << (nWhich==StarAttribute::SDRATTR_TRANSFORMREF1X ? "sdr[transRef1X]" : "sdr[transRef1Y]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_TRANSFORMREF2X: // metric
  case StarAttribute::SDRATTR_TRANSFORMREF2Y:
    f << (nWhich==StarAttribute::SDRATTR_TRANSFORMREF2X ? "sdr[transRef2X]" : "sdr[transRef2Y]")
      << "=" << input->readLong(4) << ",";
    break;
  case StarAttribute::SDRATTR_TEXTDIRECTION: // uint16
    f << "sdr[textDirection]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE2:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE3:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE4:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE5:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE6:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE7:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE8:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE9:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE10:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE11:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE12:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE13:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE14:
  case StarAttribute::SDRATTR_NOTPERSISTRESERVE15:
    f << "sdr[notPersistReserve" << nWhich-StarAttribute::SDRATTR_NOTPERSISTRESERVE2+2 << ",";
    break;

  case StarAttribute::SDRATTR_GRAFRED: // signed percent
  case StarAttribute::SDRATTR_GRAFGREEN:
  case StarAttribute::SDRATTR_GRAFBLUE:
    f << (nWhich==StarAttribute::SDRATTR_GRAFRED ? "graf[red]" : nWhich==StarAttribute::SDRATTR_GRAFGREEN ? "graf[green]" : "graf[blue]")
      << "=" << input->readLong(2) << ",";
    break;
  case StarAttribute::SDRATTR_GRAFLUMINANCE: // signed percent
  case StarAttribute::SDRATTR_GRAFCONTRAST:
    f << (nWhich==StarAttribute::SDRATTR_GRAFLUMINANCE ? "graf[luminance]" : "graf[contrast]")
      << "=" << input->readLong(2) << ",";
    break;
  case StarAttribute::SDRATTR_GRAFGAMMA: // uint32
    f << "graf[gamma]=" << input->readULong(4) << ",";
    break;
  case StarAttribute::SDRATTR_GRAFTRANSPARENCE: // percent
    f << "graf[transparence]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_GRAFINVERT: // onOff
    f << "graf[invert]=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_GRAFMODE: // enum
    f << "graf[mode]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_GRAFCROP: {
    f << "graf[crop],";
    if (nVers==0) break; // clone(0)
    int32_t top, left, right, bottom;
    *input >> top >> left >> right >> bottom;
    f << "crop=" << left << "x" << top << "<->" << right << "x" << bottom << ",";
    break;
  }
  case StarAttribute::SDRATTR_GRAFRESERVE3:
  case StarAttribute::SDRATTR_GRAFRESERVE4:
  case StarAttribute::SDRATTR_GRAFRESERVE5:
  case StarAttribute::SDRATTR_GRAFRESERVE6:
    f << "grafReserve" << nWhich-StarAttribute::SDRATTR_GRAFRESERVE3+3;
    break;

  case StarAttribute::SDRATTR_3DOBJ_PERCENT_DIAGONAL: // uint16
  case StarAttribute::SDRATTR_3DOBJ_BACKSCALE:
  case StarAttribute::SDRATTR_3DOBJ_NORMALS_KIND:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_PERCENT_DIAGONAL ? "obj3d[percentDiag]" :
          nWhich==StarAttribute::SDRATTR_3DOBJ_BACKSCALE ? "obj3d[backScale]" : "obj3d[normalKind]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_DEPTH: // uint32
  case StarAttribute::SDRATTR_3DOBJ_HORZ_SEGS:
  case StarAttribute::SDRATTR_3DOBJ_VERT_SEGS:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_DEPTH ? "obj3d[depth]" :
          nWhich==StarAttribute::SDRATTR_3DOBJ_HORZ_SEGS ? "obj3d[horizSegs]" : "obj3d[verticSegs]")
      << "=" << input->readULong(4) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_END_ANGLE: // uint32
    f << "obj3d[endAngle]=" << input->readULong(4) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_DOUBLE_SIDED: // bool
  case StarAttribute::SDRATTR_3DOBJ_NORMALS_INVERT:
  case StarAttribute::SDRATTR_3DOBJ_SHADOW_3D:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_DOUBLE_SIDED ? "obj3d[doubleSided]":
          nWhich==StarAttribute::SDRATTR_3DOBJ_NORMALS_INVERT ? "obj3d[normalInvert]" : "obj3d[shadow]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_X: // uint16
  case StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_Y:
  case StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_X ? "obj3d[textProjX]" :
          nWhich==StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_Y ? "obj3d[textProjX]" : "obj3d[intensitySpecular]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_MAT_COLOR: // SvxColorItem
  case StarAttribute::SDRATTR_3DOBJ_MAT_EMISSION:
  case StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR: {
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_MAT_COLOR ? "obj3d[matColor]" :
          nWhich==StarAttribute::SDRATTR_3DOBJ_MAT_EMISSION ? "obj3d[matEmission]" : "obj3d[matSpecular]") << "=";
    STOFFColor col;
    if (!input->readColor(col)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
      f << "###color";
      break;
    }
    f << col << ",";
    break;
  }
  case StarAttribute::SDRATTR_3DOBJ_TEXTURE_KIND: // uint16
  case StarAttribute::SDRATTR_3DOBJ_TEXTURE_MODE:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_TEXTURE_KIND ? "obj3d[textureKind" : "obj3d[textureMode")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_TEXTURE_FILTER: // bool
  case StarAttribute::SDRATTR_3DOBJ_SMOOTH_NORMALS:
  case StarAttribute::SDRATTR_3DOBJ_SMOOTH_LIDS:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_TEXTURE_FILTER ? "obj3d[textureFilter]" :
          nWhich==StarAttribute::SDRATTR_3DOBJ_SMOOTH_NORMALS ? "obj3d[smoothNormals]" : "obj3d[smoothLids]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_CHARACTER_MODE: // bool
  case StarAttribute::SDRATTR_3DOBJ_CLOSE_FRONT:
  case StarAttribute::SDRATTR_3DOBJ_CLOSE_BACK:
    f << (nWhich==StarAttribute::SDRATTR_3DOBJ_CHARACTER_MODE ? "obj3d[charMode]" :
          nWhich==StarAttribute::SDRATTR_3DOBJ_CLOSE_FRONT ? "obj3d[closeFront]" : "obj3d[closeBack]")
      << "=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_06:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_07:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_08:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_09:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_10:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_11:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_12:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_13:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_14:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_15:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_16:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_17:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_18:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_19:
  case StarAttribute::SDRATTR_3DOBJ_RESERVED_20:
    f << "obj3dReserved" << nWhich-StarAttribute::SDRATTR_3DOBJ_RESERVED_06+6 << ",";
    break;

  case StarAttribute::SDRATTR_3DSCENE_PERSPECTIVE: // uint16
    f << "scene3d[perspective]=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_3DSCENE_DISTANCE: // uint32
  case StarAttribute::SDRATTR_3DSCENE_FOCAL_LENGTH:
    f << (nWhich==StarAttribute::SDRATTR_3DSCENE_DISTANCE ? "scene3d[distance]" : "scene3d[focalLength]")
      << "=" << input->readULong(4) << ",";
    break;
  case StarAttribute::SDRATTR_3DSCENE_TWO_SIDED_LIGHTING: // bool
    f << "scene3d[twoSidedLighting]=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1: // SvxColorItem
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_2:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_3:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_4:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_5:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_6:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_7:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_8: {
    f << "scene3d[lightColor" << nWhich-StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1+1 << "]=";
    STOFFColor col;
    if (!input->readColor(col)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
      f << "###color";
      break;
    }
    f << col << ",";
    break;
  }
  case StarAttribute::SDRATTR_3DSCENE_AMBIENTCOLOR: {  // SvxColorItem
    f << "scene3d[ambientColor]=";
    STOFFColor col;
    if (!input->readColor(col)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a color\n"));
      f << "###color";
      break;
    }
    f << col << ",";
    break;
  }
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_1: // bool
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_2:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_3:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_4:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_5:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_6:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_7:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTON_8:
    f << "scene3d[lightOn" << nWhich-StarAttribute::SDRATTR_3DSCENE_LIGHTON_1+1 << "]=" << input->readULong(1) << ",";
    break;
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1: // SvxVector3DItem (Vector3d)
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_2:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_3:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_4:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_5:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_6:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_7:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_8:
    f << "scene3d[lightDir" << nWhich-StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1+1 << "=";
    for (int i=0; i<3; ++i) {
      double coord;
      *input >> coord;
      f << coord << (i==2 ? "," : "x");
    }
    break;
  case StarAttribute::SDRATTR_3DSCENE_SHADOW_SLANT: // uint16
  case StarAttribute::SDRATTR_3DSCENE_SHADE_MODE:
    f << (nWhich==StarAttribute::SDRATTR_3DSCENE_SHADOW_SLANT ? "scene3d[shadowSlant]" : "scene3d[shadowMode]")
      << "=" << input->readULong(2) << ",";
    break;
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_01:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_02:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_03:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_04:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_05:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_06:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_07:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_08:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_09:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_10:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_11:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_12:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_13:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_14:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_15:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_16:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_17:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_18:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_19:
  case StarAttribute::SDRATTR_3DSCENE_RESERVED_20:
    f << "3dsceneReserved" << nWhich-StarAttribute::SDRATTR_3DSCENE_RESERVED_01+1 << ",";
    break;
  default:
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: reading not format attribute is not implemented\n"));
    f << "#unimplemented[wh=" << std::hex << nWhich << std::dec << ",";
  }
  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: read too much data\n"));
    f << "###too much,";
    ascFile.addDelimiter(input->tell(), '|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarAttributeManager::readBrushItem(StarZone &zone, int nVers, long /*endPos*/, libstoff::DebugStream &f)
{
  STOFFInputStreamPtr input=zone.input();
  STOFFColor color;
  if (!input->readColor(color)) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readBrushItem: can not read color\n"));
    f << "###color,";
    return false;
  }
  else if (!color.isWhite())
    f << "color=" << color << ",";
  if (!input->readColor(color)) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readBrushItem: can not read fill color\n"));
    f << "###fillcolor,";
    return false;
  }
  else if (!color.isWhite())
    f << "fillcolor=" << color << ",";
  f << "nStyle=" << input->readULong(1) << ",";
  if (nVers<1) return true;
  int doLoad=(int) input->readULong(2);
  if (doLoad & 1) { // TODO: svx_frmitems.cxx:1948
    STOFF_DEBUG_MSG(("StarAttributeManager::readBrushItem: do not know how to load graphic\n"));
    f << "###graphic,";
    return false;
  }
  if (doLoad & 2) {
    librevenge::RVNGString link;
    if (!zone.readString(link)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readBrushItem: can not find the link\n"));
      f << "###link,";
      return false;
    }
    if (!link.empty())
      f << "link=" << link.cstr() << ",";
  }
  if (doLoad & 4) {
    librevenge::RVNGString filter;
    if (!zone.readString(filter)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readBrushItem: can not find the filter\n"));
      f << "###filter,";
      return false;
    }
    if (!filter.empty())
      f << "filter=" << filter.cstr() << ",";
  }
  f << "nPos=" << input->readULong(1) << ",";
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab: