// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2006 Uppsala University and Aalborg University.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
 */

#ifndef XMLWRITER_H
#define XMLWRITER_H

#include "utap/common.hpp"
#include "utap/document.hpp"

#include <libxml/encoding.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

#include <stdexcept>

namespace UTAP {
xmlChar* ConvertInput(const char* in, const char* encoding);

/** Errors produced by the underlying writer (most likely due to runtime/OS issues) */
class XMLWriterError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class XMLWriter
{
public:                      // was private - needed for derived class SBMLtoXMLWriter
    xmlTextWriterPtr writer; /**< The underlying xmlTextWriter */
    Document& doc;           /**< The document to write */
    std::map<int, int> selfLoops;

    void startDocument();
    void endDocument();
    void startElement(const char* element);
    void endElement();
    void writeElement(const char* name, const char* content);
    void writeAttribute(const char* name, const char* value);
    void writeString(const char* content);
    void xmlwriteString(const xmlChar* content);

    void taTempl(const Template& templ);
    void location(const Location& loc);
    void init(const Template& templ);
    void name(const Location& state, int x, int y);
    void writeStateAttributes(const Location& state, int x, int y);
    void transition(const Edge& edge);
    void nail(int x, int y);

    void label(const char* kind, std::string data, int x, int y);
    int source(const Edge& edge);
    int target(const Edge& edge);
    void selfLoop(int loc, double initialAngle, const Edge& edge);
    void labels(int x, int y, const Edge& edge);

    void declaration();
    std::string getChanPriority() const;
    void system_instantiation();

    // public:
    XMLWriter(xmlTextWriterPtr writer, Document& doc);
    virtual ~XMLWriter();
    void project();
};
}  // namespace UTAP

#endif /* XMLWRITER_H */
