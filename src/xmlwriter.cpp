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

#include "utap/xmlwriter.hpp"
#include "utap/utap.hpp"  // writeXMLFile

#include <sstream>
#include <string_view>

#include <cmath>    // M_PI
#include <cstring>  // strlen

namespace UTAP {
using namespace Constants;

static constexpr auto MY_ENCODING = "utf-8";        // xml encoding
static constexpr auto ERR_STATE_COLOR = "#ff6666";  // pink
static constexpr auto SELF_LOOP_RADIUS = 80;
static constexpr auto STEP = 120;

static const auto NTA = (const xmlChar*)"nta";
static const auto DTD = (const xmlChar*)"-//Uppaal Team//DTD Flat System 1.6//EN";
static const auto URL = (const xmlChar*)"https://www.it.uu.se/research/group/darts/uppaal/flat-1_6.dtd";
static const auto WINDENT = (const xmlChar*)"  ";

template <typename T>
static std::string concat(std::string_view s, T value)
{
    std::ostringstream o;
    o << s << value;
    return o.str();
}

XMLWriter::XMLWriter(xmlTextWriterPtr writer, Document& doc): writer{writer}, doc{doc} {}

XMLWriter::~XMLWriter() { xmlFreeTextWriter(writer); }

/* Starts an element named "element" with no content,
 * as child of the current element or
 * as root (if it's the first element)*/
void XMLWriter::startElement(const char* element)
{
    if (xmlTextWriterStartElement(writer, (const xmlChar*)element) < 0) {
        throw XMLWriterError("StartElement");
    }
}

/* Closes the current element. */
void XMLWriter::endElement()
{
    if (xmlTextWriterEndElement(writer) < 0) {
        throw XMLWriterError("EndElement");
    }
}

/* Writes an element named "name", with the content
 * "content" as child of the current element. */
void XMLWriter::writeElement(const char* name, const char* content)
{
    if (xmlTextWriterWriteElement(writer, (const xmlChar*)name, (const xmlChar*)content) < 0) {
        throw XMLWriterError("Element");
    }
}

/* Writes a String in the current element. */
void XMLWriter::writeString(const char* data)
{
    if (xmlTextWriterWriteString(writer, (const xmlChar*)data) < 0) {
        throw XMLWriterError("String");
    }
}

/* Write a String in the current element. */
void XMLWriter::xmlwriteString(const xmlChar* data)
{
    if (xmlTextWriterWriteString(writer, data) < 0) {
        throw XMLWriterError("String");
    }
}

/* Adds an attribute with name "name" and value "value"
 * to the current element. */
void XMLWriter::writeAttribute(const char* name, const char* value)
{
    if (xmlTextWriterWriteAttribute(writer, (const xmlChar*)name, (const xmlChar*)value) < 0) {
        throw XMLWriterError("Attribute");
    }
}

/** Parses optional declaration. */
void XMLWriter::declaration()
{
    auto globalDeclarations = doc.get_globals().str(true);
    globalDeclarations += "\n";
    globalDeclarations += getChanPriority();
    globalDeclarations += " ";
    writeElement("declaration", globalDeclarations.c_str());
}

std::string XMLWriter::getChanPriority() const
{
    const auto& prs = doc.get_chan_priorities();
    auto str = std::string{};
    if (!prs.empty()) {
        str += "// channel priorities\n";
        for (const auto& pr : prs) {
            str += pr.str();
            str += ";\n";
        }
    }
    return str;
}

/* writes a "label" element with the "kind", "x" and "y" attributes
 * an with the "data" content. */
void XMLWriter::label(const char* kind, std::string data, int x, int y)
{
    if (data == "1") {
        return;
    }
    // TODO: fix the strg conversion instead of manipulating strings
    if (data.substr(0, 5) == "1 && ") {
        data = data.substr(5, data.size() - 5);
    }
    xmlChar* tmp = ConvertInput(data.c_str(), MY_ENCODING);
    if (tmp == nullptr) {
        return;
    }
    startElement("label");
    writeAttribute("kind", kind);
    writeAttribute("x", std::to_string(x).c_str());
    writeAttribute("y", std::to_string(y).c_str());
    xmlwriteString(tmp);
    xmlFree(tmp);
    endElement();
}

void XMLWriter::name(const Location& loc, int x, int y)
{
    const char* name = loc.uid.get_name().c_str();
    startElement("name");
    writeAttribute("x", std::to_string(x).c_str());
    writeAttribute("y", std::to_string(y).c_str());
    writeString(name);
    endElement();
}

void XMLWriter::writeStateAttributes(const Location& loc, int x, int y)
{
    const auto id = loc.nr;
    writeAttribute("id", concat("id", id).c_str());
    writeAttribute("x", std::to_string(x).c_str());
    writeAttribute("y", std::to_string(y).c_str());
}

/* writes a location */
void XMLWriter::location(const Location& loc)
{
    startElement("location");
    auto x = STEP * loc.nr;
    auto y = STEP * loc.nr;
    // identifier, x, y (attributes)
    writeStateAttributes(loc, x, y);
    if (loc.uid.get_name() == "Err") {
        writeAttribute("color", ERR_STATE_COLOR);
    }
    // name (element)
    x += 8;
    y += 8;
    name(loc, x, y);
    // invariant
    if (!loc.invariant.empty()) {
        y += 16;
        label("invariant", loc.invariant.str(), x, y);
    }
    // exponential rate
    if (!loc.exp_rate.empty()) {
        y += 16;
        label("exponentialrate", loc.exp_rate.str(), x, y);
    }
    // "committed" or "urgent" element
    if (loc.uid.get_type().is(COMMITTED)) {
        startElement("committed");
        endElement();
    } else if (loc.uid.get_type().is(URGENT)) {
        startElement("urgent");
        endElement();
    }
    endElement();  // end of the "location" element
}

/* writes the init tag */
void XMLWriter::init(const Template& templ)
{
    const auto id = static_cast<const Location*>(templ.init.get_data())->nr;
    startElement("init");
    writeAttribute("ref", concat("id", id).c_str());
    endElement();
}

/* writes the source of the given edge */
int XMLWriter::source(const Edge& edge)
{
    int loc = edge.src->nr;
    const auto id = concat("id", loc);
    startElement("source");
    writeAttribute("ref", id.c_str());
    endElement();
    return loc;
}

/* writes the target of the given edge */
int XMLWriter::target(const Edge& edge)
{
    int loc = edge.dst->nr;
    const auto id = concat("id", loc);
    startElement("target");
    writeAttribute("ref", id.c_str());
    endElement();
    return loc;
}

void XMLWriter::selfLoop(const int loc, const double initialAngle, const Edge& edge)
{  // four loops in PI/2
    constexpr auto pi_8 = M_PI / 8;
    const auto begin = initialAngle + pi_8 * selfLoops[loc] + 0.1;
    const auto end = begin + pi_8 - 0.1;
    int l = loc * STEP;
    int x1 = l + (int)(SELF_LOOP_RADIUS * std::cos(begin));
    int y1 = l + (int)(SELF_LOOP_RADIUS * std::sin(begin));
    int x2 = l + (int)(SELF_LOOP_RADIUS * std::cos(end));
    int y2 = l + (int)(SELF_LOOP_RADIUS * std::sin(end));
    int x3 = l + (int)((SELF_LOOP_RADIUS + 10) * std::cos((begin + end) / 2));
    int y3 = l + (int)((SELF_LOOP_RADIUS + 10) * std::sin((begin + end) / 2));
    selfLoops[loc]++;

    labels(x3, y3, edge);  // TODO
    nail(x1, y1);
    nail(x2, y2);
}

void XMLWriter::nail(int x, int y)
{
    startElement("nail");
    writeAttribute("x", concat("", x).c_str());
    writeAttribute("y", concat("", y).c_str());
    endElement();
}

/* writes a transition */
void XMLWriter::transition(const Edge& edge)
{
    startElement("transition");
    // source and target
    const auto src = source(edge);
    const auto dst = target(edge);
    if (src == dst) {
        auto angle = (edge.src->uid.get_name() != "lpmin") ? (3 * M_PI_2) : M_PI;
        selfLoop(src, angle, edge);
    } else {
        int x = STEP * src;
        int y = STEP * dst;
        // labels
        labels(x, y, edge);
        nail(x, y);
    }
    endElement();  // end of the "transition" element
}

void XMLWriter::labels(int x, int y, const Edge& edge)
{
    if (!edge.select.empty()) {
        auto str = edge.select[0].get_name() + " : ";
        if (edge.select[0].get_type().size() > 0 && edge.select[0].get_type()[0].size() > 0) {
            str += edge.select[0].get_type()[0].get_label(0);
        }  // else ? should not happen
        label("select", str, x, y - 32);
    }
    if (!edge.guard.empty()) {
        label("guard", edge.guard.str(), x, y - 16);
    }
    if (!edge.sync.empty()) {
        label("synchronisation", edge.sync.str(), x, y);
    }
    if (!edge.assign.empty()) {
        label("assignment", edge.assign.str(), x, y + 16);
    }
}

/** writes a template */
void XMLWriter::taTempl(const Template& templ)
{
    if (!templ.is_TA) {
        return;
    }
    selfLoops.clear();
    const auto name = templ.uid.get_name();
    const auto parameters = templ.parameters_str();
    const auto declarations = templ.str(false);

    startElement("template");
    writeElement("name", name.c_str());
    writeElement("parameter", parameters.c_str());
    writeElement("declaration", declarations.c_str());

    // locations
    for (const auto& loc : templ.locations) {
        location(loc);
        selfLoops[loc.nr] = 0;
    }
    // initial location
    init(templ);
    // transitions
    for (const auto& e : templ.edges)
        transition(e);
    endElement();  // end of the "template" tag
}

void XMLWriter::system_instantiation()
{  // TODO proc priority
    auto str = std::string{};
    auto proc = std::string{};
    for (const Instance& p : doc.get_processes()) {
        if (p.uid.get_name() != p.templ->uid.get_name())
            str += p.uid.get_name() + " = " + p.templ->uid.get_name() + "(" + p.arguments_str() + ");\n";
        proc += p.uid.get_name() + ", ";
    }
    proc = proc.substr(0, proc.size() - 2);
    str += "system " + proc + "; ";
    writeElement("system", str.c_str());
}

/** Parse the project document. */
void XMLWriter::project()
{
    startDocument();
    startElement("nta");
    declaration();  // global declarations

    for (const Template& t : doc.get_templates())
        taTempl(t);
    system_instantiation();
    endElement();  // close the "nta" element
    endDocument();
}

void XMLWriter::startDocument()
{
    /* Starts the document with the xml default for the version,
     * encoding UTF-8 and the default for the standalone
     * declaration. */
    if (xmlTextWriterStartDocument(writer, nullptr, MY_ENCODING, nullptr) < 0) {
        throw XMLWriterError("StartDocument");
    }
    xmlTextWriterWriteDTD(writer, NTA, DTD, URL, nullptr);
    xmlTextWriterSetIndent(writer, 1);
    xmlTextWriterSetIndentString(writer, WINDENT);
}

void XMLWriter::endDocument()
{
    if (xmlTextWriterEndDocument(writer) < 0) {
        throw XMLWriterError("EndDocument");
    }
}

/**
 * ConvertInput:
 * @in: string in a given encoding
 * @encoding: the encoding used
 *
 * Converts @in into UTF-8 for processing with libxml2 APIs
 *
 * Returns the converted UTF-8 string, or nullptr in case of error.
 */
xmlChar* ConvertInput(const char* in, const char* encoding)
{
    if (in == nullptr)
        return nullptr;

    const auto* handler = xmlFindCharEncodingHandler(encoding);
    if (handler == nullptr) {
        printf("ConvertInput: no encoding handler found for '%s'\n", (encoding != nullptr) ? encoding : "");
        return nullptr;
    }

    const auto size = std::strlen(in) + 1;
    auto out_size = static_cast<int>(size * 2 - 1);  // int is required by handler->input(...)
    auto* out = (unsigned char*)xmlMalloc(static_cast<size_t>(out_size));

    if (out != nullptr) {
        auto temp = static_cast<int>(size - 1);
        auto ret = handler->input(out, &out_size, (const xmlChar*)in, &temp);
        if ((ret < 0) || (temp - size + 1 != 0)) {
            if (ret < 0) {
                printf("ConvertInput: conversion wasn't successful.\n");
            } else {
                printf("ConvertInput: conversion wasn't successful. converted: %i octets.\n", temp);
            }
            xmlFree(out);
            out = nullptr;
        } else {
            out = (unsigned char*)xmlRealloc(out, out_size + 1);
            out[out_size] = 0; /*null terminating out */
        }
    } else {
        printf("ConvertInput: no mem\n");
    }
    return out;
}

}  // namespace UTAP

int32_t write_XML_file(const char* filename, UTAP::Document& doc)
{
    /* Create a new XmlWriter for filename, with no compression. */
    auto* writer = xmlNewTextWriterFilename(filename, 0);
    if (writer == nullptr) {
        throw UTAP::XMLWriterError{"construction"};
    }
    UTAP::XMLWriter(writer, doc).project();

    // xmlFreeTextWriter(writer);
    return 0;
}
