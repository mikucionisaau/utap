// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2010-2020 Aalborg University.
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

#include "keywords.hpp"
#include "libparser.hpp"

#include "utap/utap.hpp"

#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlstring.h>
#include <libxml/xpath.h>

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstring>  // strncmp
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace UTAP {
enum class tag_t {
    NTA,
    PROJECT,
    IMPORTS,
    DECLARATION,
    TEMPLATE,
    INSTANTIATION,
    SYSTEM,
    NAME,
    PARAMETER,
    LOCATION,
    INIT,
    TRANSITION,
    URGENT,
    COMMITTED,
    BRANCHPOINT,
    SOURCE,
    TARGET,
    LABEL,
    NAIL,
    LSC,
    TYPE,
    MODE,
    YLOCCOORD,
    LSCLOCATION,
    PRECHART,
    INSTANCE,
    TEMPERATURE,
    MESSAGE,
    CONDITION,
    UPDATE,
    ANCHOR,
    QUERIES,
    QUERY,
    FORMULA,
    COMMENT,
    OPTION,
    RESOURCE,
    EXPECT,
    RESULT,
    DETAILS,
    SAMPLES,
    PLOT,
    TITLE,
    SERIES,
    POINT,
    NONE
};

// clang-format off
    static const auto tag_map = std::unordered_map<std::string_view, const tag_t>{
            {"nta",           tag_t::NTA},
            {"project",       tag_t::PROJECT},
            {"imports",       tag_t::IMPORTS},
            {"declaration",   tag_t::DECLARATION},
            {"template",      tag_t::TEMPLATE},
            {"instantiation", tag_t::INSTANTIATION},
            {"system",        tag_t::SYSTEM},
            {"name",          tag_t::NAME},
            {"parameter",     tag_t::PARAMETER},
            {"location",      tag_t::LOCATION},
            {"init",          tag_t::INIT},
            {"transition",    tag_t::TRANSITION},
            {"urgent",        tag_t::URGENT},
            {"committed",     tag_t::COMMITTED},
            {"branchpoint",   tag_t::BRANCHPOINT},
            {"source",        tag_t::SOURCE},
            {"target",        tag_t::TARGET},
            {"label",         tag_t::LABEL},
            {"nail",          tag_t::NAIL},
            {"lsc",           tag_t::LSC},
            {"type",          tag_t::TYPE},
            {"mode",          tag_t::MODE},
            {"yloccoord",     tag_t::YLOCCOORD},
            {"lsclocation",   tag_t::LSCLOCATION},
            {"prechart",      tag_t::PRECHART},
            {"instance",      tag_t::INSTANCE},
            {"temperature",   tag_t::TEMPERATURE},
            {"message",       tag_t::MESSAGE},
            {"condition",     tag_t::CONDITION},
            {"update",        tag_t::UPDATE},
            {"anchor",        tag_t::ANCHOR},
            {"queries",       tag_t::QUERIES},
            {"query",         tag_t::QUERY},
            {"formula",       tag_t::FORMULA},
            {"comment",       tag_t::COMMENT},
            {"option",        tag_t::OPTION},
            {"resource",      tag_t::RESOURCE},
            {"expect",        tag_t::EXPECT},
            {"result",        tag_t::RESULT},
            {"details",       tag_t::DETAILS},
            {"samples",       tag_t::SAMPLES},
            {"plot",          tag_t::PLOT},
            {"series",        tag_t::SERIES}
    };
// clang-format on

/**
 * Returns true if string is zero length or contains only white spaces
 * otherwise false.
 */
static bool is_blank(std::string_view str) { return std::all_of(str.cbegin(), str.cend(), ::isspace); }

static inline bool is_blank(const xmlChar* str) { return is_blank((const char*)str); }

static inline bool is_alpha(char c) { return std::isalpha(c) != 0 || c == '_'; }

static bool is_id_char(char c) { return std::isalnum(c) != 0 || c == '_' || c == '$' || c == '#'; }

struct id_expected_error : std::logic_error
{
    id_expected_error(): std::logic_error{"Identifier expected"} {}
};

struct invalid_id_error : std::logic_error
{
    invalid_id_error(): std::logic_error{"Invalid identifier"} {}
};

struct xpath_corrupt_error : std::logic_error
{
    xpath_corrupt_error(): std::logic_error{"XPath is corrupted"} {}
};

std::string_view trim(std::string_view text)
{
    static constexpr auto ws = "\t\n\v\f\r ";
    auto res = text.substr(0, 0);
    if (auto b = text.find_first_not_of(ws); b != std::string_view::npos) {
        auto e = text.find_last_not_of(ws);
        res = text.substr(b, e - b + 1);
    }
    return res;
}

/**
 * Extracts the alpha-numerical symbol used for variable/type
 * identifiers.  Identifier starts with alpha and further might
 * contain digits, white spaces are ignored.
 * @param text a potential identifier as a character string.
 * @return valid identifier trimmed
 * @throws std::logic_error if identifier is invalid.
 */
static std::string_view symbol(std::string_view text)
{
    text = trim(text);
    if (text.empty())
        throw id_expected_error{};
    if (!is_alpha(text[0]))
        throw id_expected_error{};
    for (const auto& c : text)
        if (!is_id_char(c))
            throw invalid_id_error{};
    return text;
}

/**
 * Path to current node. This path also contains information about
 * the left siblings of the nodes. This information is used to
 * generated an XPath expression.
 *
 * @see get()
 */
class Path
{
private:
    std::list<std::vector<tag_t>> path;

public:
    Path() { path.emplace_back(); };
    void push(tag_t tag)
    {
        path.back().push_back(tag);
        path.emplace_back();
    }
    tag_t pop()
    {
        path.pop_back();
        return path.back().back();
    }
    [[nodiscard]] std::string str(tag_t tag = tag_t::NONE) const;
};

static inline size_t count(const std::vector<tag_t>& level, tag_t tag)
{
    return static_cast<size_t>(std::count(std::begin(level), std::end(level), tag));
}

/** Returns the XPath encoding of the current path. */
[[nodiscard]] std::string Path::str(tag_t tag) const
{
    std::ostringstream str;
    for (auto&& level : path) {
        if (level.empty())
            break;
        switch (level.back()) {
        case tag_t::NTA: str << "/nta"; break;
        case tag_t::PROJECT: str << "/project"; break;
        case tag_t::IMPORTS: str << "/imports"; break;
        case tag_t::DECLARATION: str << "/declaration"; break;
        case tag_t::TEMPLATE: str << "/template[" << count(level, tag_t::TEMPLATE) << "]"; break;
        case tag_t::INSTANTIATION: str << "/instantiation"; break;
        case tag_t::SYSTEM: str << "/system"; break;
        case tag_t::NAME: str << "/name"; break;
        case tag_t::PARAMETER: str << "/parameter"; break;
        case tag_t::LOCATION: str << "/location[" << count(level, tag_t::LOCATION) << "]"; break;
        case tag_t::BRANCHPOINT: str << "/branchpoint[" << count(level, tag_t::BRANCHPOINT) << "]"; break;
        case tag_t::INIT: str << "/init"; break;
        case tag_t::TRANSITION: str << "/transition[" << count(level, tag_t::TRANSITION) << "]"; break;
        case tag_t::LABEL: str << "/label[" << count(level, tag_t::LABEL) << "]"; break;
        case tag_t::URGENT: str << "/urgent"; break;
        case tag_t::COMMITTED: str << "/committed"; break;
        case tag_t::SOURCE: str << "/source"; break;
        case tag_t::TARGET: str << "/target"; break;
        case tag_t::NAIL: str << "/nail[" << count(level, tag_t::NAIL) << "]"; break;
        case tag_t::LSC: str << "/lscTemplate[" << count(level, tag_t::LSC) << "]"; break;
        case tag_t::TYPE: str << "/type"; break;
        case tag_t::MODE: str << "/mode"; break;
        case tag_t::YLOCCOORD: str << "/ylocoord[" << count(level, tag_t::YLOCCOORD) << "]"; break;
        case tag_t::LSCLOCATION: str << "/lsclocation"; break;
        case tag_t::PRECHART: str << "/prechart"; break;
        case tag_t::INSTANCE: str << "/instance[" << count(level, tag_t::INSTANCE) << "]"; break;
        case tag_t::TEMPERATURE: str << "/temperature[" << count(level, tag_t::TEMPERATURE) << "]"; break;
        case tag_t::MESSAGE: str << "/message[" << count(level, tag_t::MESSAGE) << "]"; break;
        case tag_t::CONDITION: str << "/condition[" << count(level, tag_t::CONDITION) << "]"; break;
        case tag_t::UPDATE: str << "/update[" << count(level, tag_t::UPDATE) << "]"; break;
        case tag_t::ANCHOR: str << "/anchor[" << count(level, tag_t::ANCHOR) << "]"; break;
        case tag_t::QUERIES: str << "/queries"; break;
        case tag_t::QUERY: str << "/query[" << count(level, tag_t::QUERY) << "]"; break;
        case tag_t::FORMULA: str << "/formula"; break;
        case tag_t::COMMENT: str << "/comment"; break;
        case tag_t::OPTION: str << "/option"; break;
        case tag_t::RESOURCE: str << "/resource"; break;
        case tag_t::EXPECT: str << "/expect"; break;
        case tag_t::RESULT: str << "/result"; break;
        case tag_t::DETAILS: str << "/details"; break;
        case tag_t::SAMPLES: str << "/samples"; break;
        default:
            /* Strange tag on stack */
            throw xpath_corrupt_error{};
        }
        if (level.back() == tag) {
            break;
        }
    }
    return str.str();
}

/**
 * Implements a recursive descent parser for UPPAAL XML documents.
 * Uses the xmlTextReader API from libxml2.
 */
class XMLReader
{
private:
    using elementmap_t = std::map<std::string, std::string>;
    using xmlTextReader_ptr = std::unique_ptr<xmlTextReader, decltype(&xmlFreeTextReader)>;
    xmlTextReader_ptr reader; /**< The underlying xmlTextReader */
    elementmap_t names;       /**< Map from id to name */
    ParserBuilder& parser;    /**< The parser builder to which to push the model. */
    bool newxta;              /**< True if we should use new syntax. */
    Path path;
    bool nta;                /**< True if the enclosing tag is "nta" (false if it is "project") */
    int bottomPrechart;      /**< y location of the prechart bottom */
    std::string currentType; /**< type of the current LSC template */
    std::string currentMode; /**< mode of the current LSC template */

    [[nodiscard]] tag_t getElement() const;
    /** Reads an attribute value of the currently parsed tag with manual deallocation.
     * @param name the name of the XML tag attribute
     * @return the value of the attribute, remember to xmlFree() it!
     */
    char* getAttribute(const char* name) const;
    /** Reads an attribute value of the currently parsed tag with automatic deallocation.
     * @param name the name of the XML tag attribute
     * @return the value of the attribute.
     */
    std::string getAttributeStr(std::string_view name) const;

    bool isEmpty() const;
    int getNodeType() const;
    void read();
    bool begin(tag_t, bool skipEmpty = true);
    bool end(tag_t);
    /** skips the content until tag is closed and then looks ahead */
    void close(tag_t tag)
    {
        if (!isEmpty()) {
            while (!end(tag))
                read();
        }
        read();
    }
    /** calls fn zero or one times unless closing tag is found */
    template <typename Fn>
    void zero_or_one(tag_t closing_tag, Fn&& fn)
    {
        if (!end(closing_tag))
            fn();
    }
    /** calls fn zero or more times unless closing tag is found */
    template <typename Fn>
    void zero_or_more(tag_t closing_tag, Fn&& fn)
    {
        while (!end(closing_tag) && fn())
            ;
    }
    /** Returns the name of a location. */
    const std::string& get_name(const char* id) const;
    /** Invokes the bison generated parser to parse the given string. */
    int parse(const xmlChar*, XTAPart syntax);
    /** Parse optional declaration. */
    bool declaration();
    /** Parse optional label. */
    bool label(bool required = false, const std::string& kind = "");
    int invariant();
    /** Parse optional committed tag. */
    bool committed();
    /** Parse optional urgent tag. */
    bool urgent();
    /** Parse optional location. */
    bool location();
    /** Parse optional branchpoint. */
    bool branchpoint();
    /** Parse optional init tag. The caller must define a position to
     * which any error messages are attached.
     */
    bool init();
    /** Parse optional name tag. */
    std::string name(bool instanceLine = false);
    std::string readString(tag_t tag, bool instanceLine = false);
    std::string readText(bool instanceLine = false);
    int readNumber();
    /** Parse obligatory source tag. */
    std::string source();
    /** Parse obligatory target tag. */
    std::string target();
    /** Parse optional transition. */
    bool transition();
    /** Parse optional template. */
    bool templ();
    /** Parses an optional parameter tag and returns the number of parameters. */
    int parameter();
    /** Parse optional instantiation tag. */
    bool instantiation();
    /** Parse required system tag. */
    void system();
    std::string reference(const std::string& attributeName);

    // LSC elements:
    /** Parse optional LSC template. */
    bool lscTempl();
    /** Parse obligatory anchor tag for update. */
    std::string anchor();
    /** Parse obligatory anchor tag for condition. */
    std::vector<std::string> anchors();
    /** Parse optional type tag. */
    std::string type();
    /** Parse optional mode tag. */
    std::string mode();
    /** Parse required lsclocation tag for the prechart (bottom location or the messages) */
    int lscLocation();
    std::string temperature();
    bool yloccoord();
    bool instance();
    bool prechart();
    bool message();
    bool condition();
    bool update();

    // integrated query elements:
    /** Parse optional enclosed queries inside the model file. */
    bool queries();
    bool model_options();
    bool query();
    bool formula();
    bool comment();
    bool option();
    bool expectation();
    bool result();

public:
    XMLReader(xmlTextReaderPtr reader, ParserBuilder& parser, bool newxta):
        reader(reader, xmlFreeTextReader), parser{parser}, newxta{newxta}
    {
        read();
    }
    /** Parse the project document (either NTA or PROJECT tag). */
    void project();
};

static const auto non_unique_id = std::string{"$Non-unique_id_attribute_value: "};

/** Returns the type of the current node. */
int XMLReader::getNodeType() const { return xmlTextReaderNodeType(reader.get()); }

/**
 * Returns the tag of the current element. Throws an exception if
 * the tag is not known.
 */
tag_t XMLReader::getElement() const
{
    const char* element = (const char*)xmlTextReaderConstLocalName(reader.get());
    const auto tag = tag_map.find(element);
    if (tag == std::end(tag_map)) {
        /* Unknown element. */
        return tag_t::NONE;
    }
    return tag->second;
}

char* XMLReader::getAttribute(const char* name) const
{
    return (char*)xmlTextReaderGetAttribute(reader.get(), (xmlChar*)name);
}

std::string XMLReader::getAttributeStr(std::string_view name) const
{
    char* value = getAttribute(name.data());
    auto res = std::string{value};
    xmlFree(value);
    return res;
}

/** Returns true if the current element is an empty element. */
bool XMLReader::isEmpty() const
{
    int res = xmlTextReaderIsEmptyElement(reader.get());
    assert(0 <= res);
    assert(res <= 1);
    return res == 1;
}

/**
 * Read until start element. Returns true if that element has the
 * given tag. If skipEmpty is true, empty elements with the given
 * tag are ignored.
 */
bool XMLReader::begin(tag_t tag, bool skipEmpty)
{
    for (;;) {
        int node_type = getNodeType();
        while (node_type != XML_READER_TYPE_ELEMENT) {
            read();
            node_type = getNodeType();
        }
        tag_t elem = getElement();
        if (elem != tag) {
            // if the tag was not recognized, try skipping over it until
            // an end element is found with unknown tag.
            if (elem == tag_t::NONE) {
                end(tag_t::NONE);
                read();
                continue;
            }
            return false;
        }
        if (!skipEmpty || !isEmpty())
            return true;
        read();
    }
}
/**
 * Is end of the tag in xml ?
 * @param tag - XML tag
 * @return True - if </...> tag found
 * Ignores whitespace
 */
bool UTAP::XMLReader::end(UTAP::tag_t tag)
{
    int node_type = getNodeType();
    // Ignore whitespace
    while (node_type == XML_READER_TYPE_WHITESPACE || node_type == XML_READER_TYPE_SIGNIFICANT_WHITESPACE) {
        read();
        node_type = getNodeType();
    }
    // </...> tag found
    return node_type == XML_READER_TYPE_END_ELEMENT && getElement() == tag;
}

/**
 * Advances the reader. It maintains the path to the current node.
 */
void XMLReader::read()
{
    if ((getNodeType() == XML_READER_TYPE_END_ELEMENT) || (getNodeType() == XML_READER_TYPE_ELEMENT && isEmpty())) {
        if (path.pop() != getElement()) {
            /* Path is corrupted */
            throw XMLDocError("Invalid nesting");
        }
    }
    if (xmlTextReaderRead(reader.get()) != 1) {
        /* Premature end of document. */
        throw XMLReaderError(errno, std::system_category(), "$unexpected $end");
    }

    if (getNodeType() == XML_READER_TYPE_ELEMENT) {
        path.push(getElement());
    }
}

const std::string& XMLReader::get_name(const char* id) const
{
    if (id != nullptr) {
        if (auto l = names.find(id); l != names.end())
            return l->second;
    }
    throw XMLDocError("Missing reference");
}

int XMLReader::parse(const xmlChar* text, XTAPart syntax)
{
    return parse_XTA((const char*)text, parser, newxta, syntax, path.str());
}

bool XMLReader::declaration()
{
    if (begin(tag_t::DECLARATION)) {
        read();
        if (getNodeType() == XML_READER_TYPE_TEXT) {
            parse(xmlTextReaderConstValue(reader.get()), S_DECLARATION);
        }
        return true;
    }
    return false;
}

bool XMLReader::label(bool required, const std::string& s_kind)
{
    if (begin(tag_t::LABEL)) {
        /* Get kind attribute. */
        char* kind = getAttribute("kind");
        if (kind == nullptr)
            throw TypeException("A label must have a \"kind\" attribute");
        read();
        /* Read the text and push it to the parser. */
        if (getNodeType() == XML_READER_TYPE_TEXT) {
            const xmlChar* text = xmlTextReaderConstValue(reader.get());
            static const auto map = std::map<std::string_view, XTAPart>{
                {"invariant", S_INVARIANT},  {"select", S_SELECT},     {"guard", S_GUARD},
                {"synchronisation", S_SYNC}, {"assignment", S_ASSIGN}, {"probability", S_PROBABILITY},
                {"message", S_MESSAGE},      {"update", S_UPDATE},     {"condition", S_CONDITION},
            };
            if (auto part = map.find(kind); part != map.end())
                parse(text, part->second);
        }
        xmlFree(kind);
        return true;
    } else if (required) {
        tracker.setPath(&parser, path.str());
        if (s_kind == "message")  // LSC
            parser.handle_error(TypeException{"$Message_label_is_required"});
        else if (s_kind == "update")  // LSC
            parser.handle_error(TypeException{"$Update_label_is_required"});
        else if (s_kind == "condition")  // LSC
            parser.handle_error(TypeException{"$Condition_label_is_required"});
    }
    return false;
}

int XMLReader::invariant()
{
    int result = -1;
    if (begin(tag_t::LABEL)) {
        /* Get kind attribute. */
        char* kind = getAttribute("kind");
        if (kind == nullptr)
            throw TypeException{"A label must have a \"kind\" attribute"};
        read();
        /* Read the text and push it to the parser. */
        if (getNodeType() == XML_READER_TYPE_TEXT) {
            const xmlChar* text = xmlTextReaderConstValue(reader.get());
            auto kind_sv = std::string_view{kind};
            // This is a terrible mess but it's too badly designed
            // to fix at this moment.
            if (kind_sv == "invariant") {
                if (parse(text, S_INVARIANT) == 0)
                    result = 0;
            } else if (kind_sv == "exponentialrate") {
                if (parse(text, S_EXPONENTIAL_RATE) == 0)
                    result = 1;
            }
        }
        xmlFree(kind);
    }
    return result;
}

std::string XMLReader::name(bool instanceLine)
{
    std::string text = readString(tag_t::NAME, instanceLine);
    if (instanceLine && text.empty())
        parser.handle_error(TypeException{"$Instance_name_is_required"});
    return text;
}

std::string XMLReader::readText(bool instanceLine)
{
    if (getNodeType() == XML_READER_TYPE_TEXT) {  // text content of a node
        xmlChar* text = xmlTextReaderValue(reader.get());
        auto text_sv = std::string_view{text != nullptr ? (const char*)text : ""};
        tracker.setPath(&parser, path.str());
        tracker.increment(&parser, static_cast<uint32_t>(text_sv.size()));
        try {
            auto id = (instanceLine) ? text_sv : symbol(text_sv);
            if (!is_keyword(id, Syntax::OLD_PROPERTY)) {
                auto res = std::string{id};
                xmlFree(text);
                return res;
            }
            parser.handle_error(TypeException{"$Keywords_are_not_allowed_here"});
        } catch (std::logic_error& str) {
            parser.handle_error(TypeException{str.what()});
        }
        xmlFree(text);
    }
    return "";
}

int XMLReader::readNumber()
{
    read();
    if (getNodeType() == XML_READER_TYPE_TEXT) {  // text content of a node
        tracker.setPath(&parser, path.str());
        xmlChar* text = xmlTextReaderValue(reader.get());
        const char* pc = (const char*)text;
        const auto len = static_cast<uint32_t>(std::strlen(pc));
        tracker.increment(&parser, len);
        try {
            int value;
            if (auto [p, ec] = std::from_chars(pc, pc + len, value); ec != std::errc{})
                throw std::logic_error{std::make_error_code(ec).category().name()};
            xmlFree(text);
            return value;
        } catch (const char* str) {
            parser.handle_error(TypeException{str});
        }
        xmlFree(text);
    }
    return -1;
}

std::string XMLReader::readString(tag_t tag, bool instanceLine)
{
    if (begin(tag)) {
        read();
        return readText(instanceLine);
    }
    return "";
}

std::string XMLReader::type() { return readString(tag_t::TYPE); }

std::string XMLReader::mode() { return readString(tag_t::MODE); }

int XMLReader::lscLocation()
{
    int n = -1;
    if (begin(tag_t::LSCLOCATION)) {
        n = readNumber();
    }
    if (n == -1)
        throw XMLDocError("Missing LSC location");
    return n;
}

bool XMLReader::committed()
{
    if (begin(tag_t::COMMITTED, false)) {
        read();
        return true;
    }
    return false;
}

bool XMLReader::urgent()
{
    if (begin(tag_t::URGENT, false)) {
        read();
        return true;
    }
    return false;
}

bool XMLReader::location()
{
    bool l_invariant = false;
    bool l_exponentialRate = false;

    if (begin(tag_t::LOCATION, false)) {
        try {
            std::string l_path = path.str(tag_t::LOCATION);
            /* Extract ID attribute. */
            auto l_id = getAttributeStr("id");
            if (is_blank(l_id))
                throw TypeException{"Every location must have a unique id attribute value"};
            read();
            /* Get name of the location. */
            std::string l_name = name();
            /* Read the invariant. */
            while (begin(tag_t::LABEL)) {
                int res = invariant();
                l_invariant |= res == 0;
                l_exponentialRate |= res == 1;
            }
            /* Is the location urgent or committed? */
            bool l_urgent = urgent();
            bool l_committed = committed();

            // anonymous locations get an internal name based on the ID
            if (is_blank(l_name))
                l_name = "_" + l_id;
            /* Remember the mapping from id to name */
            if (auto [_, ins] = names.insert_or_assign(l_id, l_name); !ins)
                parser.handle_warning(TypeException{non_unique_id + l_id});

            /* Any error messages generated by any of the
             * procStateXXX calls must be attributed to the state
             * element. To do this, we add a dummy position of
             * length 1.
             */
            tracker.setPath(&parser, l_path);
            tracker.increment(&parser, 1);

            /* Push location to parser builder. */
            parser.proc_location(l_name, l_invariant, l_exponentialRate);
            if (l_committed)
                parser.proc_location_commit(l_name);
            if (l_urgent)
                parser.proc_location_urgent(l_name);
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

/** Parse optional instance. */
bool XMLReader::instance()
{
    if (begin(tag_t::INSTANCE, false)) {
        try {
            auto i_path = path.str(tag_t::INSTANCE);
            /* Extract ID attribute. */
            auto i_id = getAttributeStr("id");
            read();
            if (is_blank(i_id))
                throw TypeException{"Instance tag must have a unique \"id\" attribute"};

            /* Get name of the instance. */
            tracker.setPath(&parser, i_path);
            tracker.increment(&parser, 1);
            auto i_name = name(true);

            /* Remember the mapping from id to name */
            if (auto [_, ins] = names.insert_or_assign(i_id, i_name); !ins)
                parser.handle_warning(TypeException{non_unique_id + i_id});

            /* Any error messages generated by the
             * proc_instance_line call must be attributed to the
             * instance line element. To do this, we add a dummy
             * position of length 1.
             */
            tracker.setPath(&parser, i_path);
            tracker.increment(&parser, 1);
            /* Push instance to parser builder. */
            parser.proc_instance_line();
            parse((xmlChar*)i_name.c_str(), S_INSTANCE_LINE);
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

/** Parse optional yloccoord */
bool XMLReader::yloccoord()
{
    if (begin(tag_t::YLOCCOORD, false)) {
        read();  // used only for the GUI
        return true;
    }
    return false;
}

std::string XMLReader::temperature()
{
    if (begin(tag_t::TEMPERATURE, false)) {
        read();
        /* Get the temperature of the condition */
        return readText();
    }
    throw TypeException{"Missing temperature"};
}

bool XMLReader::prechart()
{
    if (begin(tag_t::PRECHART, false)) {
        try {
            std::string p_path = path.str(tag_t::PRECHART);
            /* Get the bottom location number */
            read();
            bottomPrechart = lscLocation();
            if (strcasecmp(currentType.c_str(), "existential") == 0) {
                tracker.setPath(&parser, p_path);
                tracker.increment(&parser, 1);
                parser.handle_error(TypeException{"$Existential_charts_must_not_have_prechart"});
            }
            parser.prechart_set(true);
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    } else {
        bottomPrechart = -1;
        parser.prechart_set(false);
    }
    return false;
}

bool XMLReader::message()
{
    if (begin(tag_t::MESSAGE)) {
        /* Add dummy position mapping to the message element. */
        try {
            std::string m_path = path.str(tag_t::MESSAGE);
            read();
            std::string from = source();
            std::string to = target();
            int location = lscLocation();
            bool pch = (location < bottomPrechart);
            tracker.setPath(&parser, m_path);
            tracker.increment(&parser, 1);
            parser.proc_message(from.c_str(), to.c_str(), location, pch);
            tracker.setPath(&parser, m_path);
            tracker.increment(&parser, 1);
            label(true, "message");
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

bool XMLReader::condition()
{
    if (begin(tag_t::CONDITION)) {
        try {
            std::string c_path = path.str(tag_t::CONDITION);
            read();

            std::vector<std::string> instance_anchors = anchors();
            int location = lscLocation();
            bool pch = (location < bottomPrechart);

            tracker.setPath(&parser, c_path);
            tracker.increment(&parser, 1);
            std::string temp = temperature();
            bool hot = (temp == "hot");
            parser.proc_condition(instance_anchors, location, pch, hot);

            label(true, "condition");
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

bool XMLReader::update()
{
    if (begin(tag_t::UPDATE)) {
        try {
            std::string u_path = path.str(tag_t::UPDATE);
            // location = atoi((char*)xmlTextReaderGetAttribute(reader, (const xmlChar*)"y"));
            // pch = (location < bottomPrechart);
            read();
            std::string instance_anchor = anchor();
            int location = lscLocation();
            bool pch = (location < bottomPrechart);

            tracker.setPath(&parser, u_path);
            tracker.increment(&parser, 1);
            parser.proc_LSC_update(instance_anchor.c_str(), location, pch);
            label(true, "update");
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

bool XMLReader::branchpoint()
{
    if (begin(tag_t::BRANCHPOINT, false)) {
        try {
            std::string b_path = path.str(tag_t::BRANCHPOINT);
            auto b_id = getAttributeStr("id");
            if (is_blank(b_id)) {
                throw TypeException{"Branchpoint must have a unique \"id\" attribute"};
            }
            /* assign an internal name based on the ID of the branchpoint. */
            std::string b_name = "_" + b_id;
            /* Remember the mapping from id to name */
            if (auto [_, ins] = names.insert_or_assign(b_id, b_name); !ins)
                parser.handle_warning(TypeException{non_unique_id + b_id});
            // FIXME: probably not necessary
            /* Any error messages generated by any of the
             * procStateXXX calls must be attributed to the state
             * element. To do this, we add a dummy position of
             * length 1.
             */
            tracker.setPath(&parser, b_path);
            tracker.increment(&parser, 1);
            /* Push branchpoint to parser builder. */
            parser.proc_branchpoint(b_name.c_str());
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        read();  // ignore any content and read next tag
        return true;
    }
    return false;
}

bool XMLReader::init()
{
    if (begin(tag_t::INIT, false)) {
        /* Get reference attribute. */
        char* ref = getAttribute("ref");
        /* Find location name for the reference. */
        if (ref != nullptr) {
            const auto name = get_name(ref);
            try {
                parser.proc_location_init(name.c_str());
            } catch (TypeException& te) {
                parser.handle_error(te);
            }
        } else {
            parser.handle_error(TypeException{"$Missing_initial_location"});
        }
        xmlFree(ref);
        read();
        return true;
    } else {
        parser.handle_error(TypeException{"$Missing_initial_location"});
    }
    return false;
}

std::string XMLReader::reference(const std::string& attributeName)
{
    char* id = getAttribute(attributeName.c_str());
    std::string name = get_name(id);
    xmlFree(id);
    read();
    return name;
}

std::string XMLReader::source()
{
    if (begin(tag_t::SOURCE, false))
        return reference("ref");
    throw TypeException{"Missing source element"};
}

std::string XMLReader::target()
{
    if (begin(tag_t::TARGET, false))
        return reference("ref");
    throw TypeException{"Missing target element"};
}

std::string XMLReader::anchor()
{
    if (begin(tag_t::ANCHOR, false))
        return reference("instanceid");
    throw TypeException{"Missing anchor element"};
}

std::vector<std::string> XMLReader::anchors()
{
    std::vector<std::string> res;
    while (begin(tag_t::ANCHOR, false))
        res.push_back(reference("instanceid"));
    if (res.empty())
        throw TypeException{"Missing anchor element"};
    return res;
}

bool XMLReader::transition()
{
    if (begin(tag_t::TRANSITION)) {
        /* Add dummy position mapping to the transition element. */
        try {
            char* type = getAttribute("controllable");
            bool control = (type == nullptr || (strcmp(type, "true") == 0));
            xmlFree(type);

            char* id = getAttribute("action");
            auto actname = std::string{id != nullptr ? id : "SKIP"};
            xmlFree(id);

            read();
            auto from = source();
            auto to = target();

            parser.proc_edge_begin(from.c_str(), to.c_str(), control, actname.c_str());
            while (label())
                ;
            while (begin(tag_t::NAIL))
                read();
            parser.proc_edge_end(from.c_str(), to.c_str());
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

int XMLReader::parameter()
{
    int count = 0;
    if (begin(tag_t::PARAMETER)) {
        read();
        if (getNodeType() == XML_READER_TYPE_TEXT) {
            count = parse(xmlTextReaderConstValue(reader.get()), S_PARAMETERS);
        }
    }
    return count;
}

bool XMLReader::templ()
{
    if (begin(tag_t::TEMPLATE)) {
        auto t_path = std::make_shared<std::string>(path.str(tag_t::TEMPLATE));
        read();
        try {
            /* Get the name and the parameters of the template. */
            std::string t_name = name();
            parameter();

            /* Push template start to parser builder. This might
             * throw a TypeException. */
            tracker.setPath(&parser, t_path);
            tracker.increment(&parser, 1);
            parser.proc_begin(t_name.c_str());

            /* Parse declarations, locations, branchpoints,
             * the init tag and the transitions of the template. */
            declaration();
            while (location())
                ;
            while (branchpoint())
                ;
            tracker.setPath(&parser, t_path);
            tracker.increment(&parser, 1);
            init();
            while (transition())
                ;

            /* Push template end to parser builder. */
            tracker.setPath(&parser, t_path);
            tracker.increment(&parser, 1);
            parser.proc_end();
        } catch (TypeException& e) {
            parser.handle_error(e);
        }

        return true;
    }
    return false;
}

bool XMLReader::lscTempl()
{
    if (begin(tag_t::LSC)) {
        std::string t_path = path.str(tag_t::LSC);
        read();
        try {
            /* Get the name and the parameters of the template. */
            std::string t_name = name();
            parser.lscTemplateNames.push_back(t_name);
            parameter();
            currentType = type();
            currentMode = mode();
            /* Push template start to parser builder. This might
             * throw a TypeException. */
            tracker.setPath(&parser, t_path);
            tracker.increment(&parser, 1);
            parser.proc_begin(t_name.c_str(), false, currentType, currentMode);

            /* Parse declarations, locations, instances, prechart
             * messages, conditions and updates */
            declaration();
            while (yloccoord())
                ;
            while (instance())
                ;
            prechart();
            while (message())
                ;
            while (condition())
                ;
            while (update())
                ;

            /* Push template end to parser builder. */
            tracker.setPath(&parser, t_path);
            tracker.increment(&parser, 1);
            parser.proc_end();
        } catch (TypeException& e) {
            parser.handle_error(e);
        }
        return true;
    }
    return false;
}

bool XMLReader::instantiation()
{
    if (begin(tag_t::INSTANTIATION, false)) {
        const auto* text = (const xmlChar*)"";
        read();
        if (getNodeType() == XML_READER_TYPE_TEXT)
            text = xmlTextReaderConstValue(reader.get());
        parse(text, S_INST);
        return true;
    }
    return false;
}

void XMLReader::system()
{
    if (begin(tag_t::SYSTEM, false)) {
        parser.system_decl_begin();

        const auto* text = (const xmlChar*)"";
        read();
        auto nodeType = getNodeType();
        if (nodeType == XML_READER_TYPE_TEXT)
            text = xmlTextReaderConstValue(reader.get());
        // if there are no non-space characters in the text (or the text is empty),
        // bison doesn't manage to properly set the position of errors,
        // leading to nonsense error placements.
        if (nodeType == XML_READER_TYPE_END_ELEMENT || is_blank(text)) {
            tracker.setPath(&parser, path.str(tag_t::SYSTEM));
            tracker.increment(&parser, 1);
            parser.handle_error(TypeException{"$syntax_error: $unexpected $end"});
            close(tag_t::SYSTEM);
            return;
        }
        parse(text, S_SYSTEM);
        parser.system_decl_end();
        close(tag_t::SYSTEM);
    } else {
        std::string s = (nta) ? path.str(tag_t::NTA) : path.str(tag_t::PROJECT);
        tracker.setPath(&parser, s);
        tracker.increment(&parser, 1);
        parser.handle_error(TypeException{"$Missing_system_tag"});
    }
}

bool XMLReader::queries()
{
    if (begin(tag_t::QUERIES, false)) {
        read();
        zero_or_one(tag_t::QUERIES, [this] { return model_options(); });
        zero_or_more(tag_t::QUERIES, [this] { return query(); });
        close(tag_t::QUERIES);
        return true;
    }
    return false;
}
bool XMLReader::query()
{
    if (begin(tag_t::QUERY, false)) {
        if (!isEmpty()) {
            read();
            parser.query_begin();
            zero_or_one(tag_t::QUERY, [this] { return formula(); });
            zero_or_one(tag_t::QUERY, [this] { return comment(); });
            zero_or_more(tag_t::QUERY, [this] { return option(); });
            zero_or_one(tag_t::QUERY, [this] { return expectation(); });
            zero_or_more(tag_t::QUERY, [this] { return result(); });
            parser.query_end();
            close(tag_t::QUERY);
        } else
            read();  // look ahead next tag
        return true;
    }
    return false;
}
bool XMLReader::formula()
{
    if (begin(tag_t::FORMULA, false)) {
        if (!isEmpty()) {
            read();
            const auto* text = xmlTextReaderConstValue(reader.get());
            if (text != nullptr)
                parser.query_formula((const char*)text, path.str(tag_t::FORMULA));
            close(tag_t::FORMULA);
        } else
            read();
        return true;
    }
    return false;
}
bool XMLReader::comment()
{
    if (begin(tag_t::COMMENT, false)) {
        if (!isEmpty()) {
            read();
            const auto* text = xmlTextReaderConstValue(reader.get());
            if (text != nullptr)
                parser.query_comment((const char*)text);
            close(tag_t::COMMENT);
        } else
            read();
        return true;
    }
    return false;
}

bool XMLReader::option()
{
    if (begin(tag_t::OPTION, false)) {
        char* key = getAttribute("key");
        char* value = getAttribute("value");
        parser.query_options(key, value);
        xmlFree(key);
        xmlFree(value);
        close(tag_t::OPTION);
        return true;
    }
    return false;
}

bool XMLReader::expectation()
{
    if (begin(tag_t::EXPECT, false)) {
        if (!isEmpty()) {
            parser.expectation_begin();
            char* outcome = getAttribute("outcome");
            char* type = getAttribute("type");
            char* value = getAttribute("value");
            parser.expectation_value(outcome, type, value);
            zero_or_more(tag_t::EXPECT, [this] {
                if (begin(tag_t::RESOURCE, false)) {
                    auto type = getAttributeStr("type");
                    auto value = getAttributeStr("value");
                    auto unit = getAttributeStr("unit");
                    parser.expect_resource(type.c_str(), value.c_str(), unit.c_str());
                    close(tag_t::RESOURCE);
                    return true;
                }
                return false;
            });
            parser.expectation_end();
            close(tag_t::EXPECT);
        } else
            read();
        return true;
    }
    return false;
}
bool XMLReader::result()
{
    if (begin(tag_t::RESULT, false)) {
        close(tag_t::RESULT);
        return true;
    }
    return false;
}

void XMLReader::project()
{
    if (!begin(tag_t::NTA) && !begin(tag_t::PROJECT))
        throw TypeException{"$Missing_nta_or_project_tag"};
    nta = begin(tag_t::NTA);  // "nta" or "project"?
    if (newxta) {
        parse((const xmlChar*)utap_builtin_declarations(), S_DECLARATION);
        parser.builtin_decl_end();
    }
    read();
    declaration();
    while (templ())
        ;
    while (lscTempl())
        ;
    instantiation();
    system();
    if ((nta && !end(tag_t::NTA)) || (!nta && !end(tag_t::PROJECT)))
        queries();
    parser.done();
}

bool XMLReader::model_options()
{
    while (begin(tag_t::OPTION)) {
        read();
        char* key = getAttribute("key");
        char* value = getAttribute("value");
        parser.model_option(key, value);
        close(tag_t::OPTION);
    }
    return true;
}
}  // namespace UTAP

using namespace UTAP;

int32_t parse_XML_fd(int fd, ParserBuilder& pb, bool newxta)
{
    xmlTextReaderPtr reader =
        xmlReaderForFd(fd, "", "", XML_PARSE_NOCDATA | XML_PARSE_NOBLANKS | XML_PARSE_HUGE | XML_PARSE_RECOVER);
    if (reader == nullptr)
        return -1;
    XMLReader(reader, pb, newxta).project();
    return 0;
}

int32_t parse_XML_file(const std::filesystem::path& path, ParserBuilder& pb, bool newxta)
{
    auto filename = path.string();
    xmlTextReaderPtr reader = xmlReaderForFile(
        filename.c_str(), "", XML_PARSE_NOCDATA | XML_PARSE_NOBLANKS | XML_PARSE_HUGE | XML_PARSE_RECOVER);
    if (reader == nullptr)
        return -1;
    XMLReader(reader, pb, newxta).project();
    return 0;
}

int32_t parse_XML_buffer(const char* buffer, ParserBuilder& pb, bool newxta)
{
    const auto length = static_cast<int>(strlen(buffer));
    xmlTextReaderPtr reader =
        xmlReaderForMemory(buffer, length, "", "", XML_PARSE_NOCDATA | XML_PARSE_HUGE | XML_PARSE_RECOVER);
    if (reader == nullptr)
        return -1;
    XMLReader(reader, pb, newxta).project();
    return 0;
}

/**
 * Get the contents of the XML element with the specified path
 * @param xmlDocPtr - The XML document.
 * @param pos - The position path
 * @return res - The contents
 */
std::string getXMLElement(xmlDocPtr docPtr, const std::string& path)  // used in verifyta.cpp
{
    std::string res;
    // Get the context
    xmlXPathContextPtr context = xmlXPathNewContext(docPtr);
    if (context == nullptr) {
        xmlFreeDoc(docPtr);
        return res;
    }
    xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar*)path.c_str(), context);
    if (result != nullptr) {
        xmlNodeSetPtr nodeset = result->nodesetval;
        if (!xmlXPathNodeSetIsEmpty(nodeset) && nodeset->nodeNr > 0) {
            // The first point of the xml node
            xmlNodePtr node = nodeset->nodeTab[0];
            xmlChar* s = xmlNodeListGetString(docPtr, node->xmlChildrenNode, 1);
            if (s != nullptr)
                res = (char*)s;
            xmlFree(s);
        }
        xmlXPathFreeObject(result);
    }
    xmlXPathFreeContext(context);
    return res;
}

/**
 * Get the contents of the XML element with the specified path
 * @param xmlBuffer - The data in the xml buffer
 * @param pos - The position path
 * @return res - The contents
 */
/* // unused
static std::string getXMLElement(const char *xmlBuffer, const std::string &path)
{
    xmlDocPtr docPtr = xmlParseMemory(xmlBuffer, strlen(xmlBuffer));
    if(docPtr) {
        std::string res = getXMLElement(docPtr,path);
        xmlFreeDoc(docPtr);
        return res;
    }
    else {
        return "";
    }
}
*/
