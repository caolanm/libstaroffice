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
 * StarWriterStruct to read/parse some basic writer structure in StarOffice documents
 *
 */
#ifndef STAR_WRITER_STRUCT
#  define STAR_WRITER_STRUCT

#include <ostream>
#include <vector>

#include "libstaroffice_internal.hxx"

namespace StarFormatManagerInternal
{
struct FormatDef;
}
class StarObject;
class StarZone;

/** \brief namespace use to keep basic writer structure */
namespace StarWriterStruct
{
/** \brief structure to store a bookmark
 */
class Bookmark
{
public:
  //! the constructor
  Bookmark() : m_shortName(""), m_name(""), m_offset(0), m_key(0), m_modifier(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Bookmark const &mark);
  //! try to read a mark
  bool read(StarZone &zone);
  //! try to read a list of bookmark
  static bool readList(StarZone &zone, std::vector<Bookmark> &markList);
  //! the shortname
  librevenge::RVNGString m_shortName;
  //! the name
  librevenge::RVNGString m_name;
  //! the offset
  int m_offset;
  //! the key
  int m_key;
  //! the modifier
  int m_modifier;
  //! the macros names
  librevenge::RVNGString m_macroNames[4];
};

/** \brief structure to store a macro in a text zone
 */
struct Macro {
public:
  //! constructor
  Macro() : m_key(0), m_scriptType(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Macro const &macro);
  //! try to read a macro: 'm'
  bool read(StarZone &zone);
  //! try to read a list of macro: 'M'
  static bool readList(StarZone &zone, std::vector<Macro> &macroLis);
  //! the key
  int m_key;
  //! the names
  librevenge::RVNGString m_names[2];
  //! the scriptType
  int m_scriptType;
};

/** \brief structure to store a mark in a text zone
 */
struct Mark {
public:
  //! constructor
  Mark() : m_type(-1), m_id(-1), m_offset(-1)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Mark const &mark);
  //! try to read a mark
  bool read(StarZone &zone);
  //! the type:  2: bookmark-start, 3:bookmark-end
  int m_type;
  //! the id
  int m_id;
  //! the offset
  int m_offset;
};

/** \brief structure to store a nodeRedline in a text zone
 */
struct NodeRedline {
public:
  //! constructor
  NodeRedline() : m_id(-1), m_offset(-1), m_flags(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, NodeRedline const &nodeRedline);
  //! try to read a nodeRedline
  bool read(StarZone &zone);
  //! the id
  int m_id;
  //! the offset
  int m_offset;
  //! the flags
  int m_flags;
};

/** \brief structure to store a TOX in a text zone
 */
struct TOX {
public:
  //! constructor
  TOX() : m_type(0), m_createType(0), m_captionDisplay(0), m_styleId(0xFFFF), m_data(0), m_formFlags(0)
    , m_title(""), m_name(""), m_OLEOptions(0), m_stringIdList(), m_styleList(), m_titleLength(), m_formatList()
  {
    for (int i=0; i<3; ++i) m_stringIds[i]=0xFFFF;
  }
  //! destructor
  ~TOX();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, TOX const &tox);
  //! try to read a TOX
  bool read(StarZone &zone, StarObject &object);
  //! try to read a list of TOX
  static bool readList(StarZone &zone, std::vector<TOX> &toxList, StarObject &object);

  //! a style
  struct Style {
    //! constructor
    Style() : m_level(0), m_names()
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Style const &style)
    {
      o << "level=" << style.m_level << ",";
      if (!style.m_names.empty()) {
        o << "names=[";
        for (size_t i=0; i<style.m_names.size(); ++i) o << style.m_names[i].cstr() << ",";
        o << "],";
      }
      return o;
    }
    //! the level
    int m_level;
    //! the list of names
    std::vector<librevenge::RVNGString> m_names;
  };
  //! the type
  int m_type;
  //! the createType
  int m_createType;
  //! the captionDisplay
  int m_captionDisplay;
  //! the string id, the seq string id, the sect string id
  int m_stringIds[3];
  //! the style id
  int m_styleId;
  //! the number of data?
  int m_data;
  //! the format flags?
  int m_formFlags;
  //! the title
  librevenge::RVNGString m_title;
  //! the name
  librevenge::RVNGString m_name;
  //! the ole options
  int m_OLEOptions;
  //! a list of template string ids
  std::vector<int> m_stringIdList;
  //! a list of style names?
  std::vector<Style> m_styleList;
  //! the title length
  long m_titleLength;
  //! the format
  std::vector<shared_ptr<StarFormatManagerInternal::FormatDef> > m_formatList;
};

/** \brief structure to store a TOX51 in a text zone
 */
struct TOX51 {
public:
  //! constructor
  TOX51() : m_typeName(""), m_type(0), m_createType(0), m_firstTabPos(0), m_title(""), m_patternList(), m_stringIdList(), m_infLevel(0)
  {
  }
  //! destructor
  ~TOX51();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, TOX51 const &tox);
  //! try to read a TOX51
  bool read(StarZone &zone, StarObject &object);
  //! try to read a list of TOX51
  static bool readList(StarZone &zone, std::vector<TOX51> &toxList, StarObject &object);

  //! the typeName
  librevenge::RVNGString m_typeName;
  //! the type
  int m_type;
  //! the createType
  int m_createType;
  //! the firstTabPos
  int m_firstTabPos;
  //! the title
  librevenge::RVNGString m_title;
  //! the pattern list
  std::vector<librevenge::RVNGString> m_patternList;
  //! a list of template string ids
  std::vector<int> m_stringIdList;
  //! the inf level
  int m_infLevel;
};

}
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
