/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020-2023 Aalborg University.
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

#ifndef INCLUDE_UTAP_DOCUMENT_FIXTURE_HPP
#define INCLUDE_UTAP_DOCUMENT_FIXTURE_HPP

#include "utap/StatementBuilder.hpp"
#include "utap/TypeChecker.hpp"
#include "utap/property.hpp"
#include "utap/utap.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstring>

/// Checks text containment in unit testing
class Contains
{
    std::string_view text;

public:
    Contains(std::string_view text): text{text} {}
    friend bool operator==(std::string_view text, const Contains& sub)
    {
        return text.find(sub.text) != std::string_view::npos;
    }
    friend bool operator!=(std::string_view text, const Contains& sub) { return !(text == sub); }
    friend std::ostream& operator<<(std::ostream& os, const Contains& sub) { return os << sub.text; }
};

inline std::string read_content(const std::string& file_name)
{
    const auto path = std::filesystem::path{MODELS_DIR} / file_name;
    auto ifs = std::ifstream{path};
    if (ifs.fail())
        throw std::system_error{errno, std::system_category(), "Failed to open " + path.string()};
    auto content = std::string{std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}};
    if (content.empty())
        throw std::runtime_error("No data was read from model file");
    return content;
}

UTAP::Document read_document(const std::string& file_name)
{
    auto doc = UTAP::Document();
    auto res = parse_XML_buffer(read_content(file_name).c_str(), doc, true);
    if (res != 0)
        throw std::logic_error("Failed to parse document");
    return doc;
}

template <typename... Args>
std::string string_format(const std::string& format, Args... args)
{
    using namespace std::string_literals;
    auto size = std::snprintf(nullptr, 0, format.c_str(), args...);
    if (size <= 0)
        throw std::logic_error("Failed to format: "s + std::strerror(errno));
    auto res = std::string(size, ' ');
    if (auto s = std::snprintf(&res[0], size + 1, format.c_str(), args...); s != size)
        throw std::logic_error("Failed to format: "s + std::strerror(errno));
    return res;
}

std::string replace_all(std::string text, const std::string& what, const std::string& with)
{
    for (auto pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + 1))
        text.replace(pos, what.length(), with);
    return text;
}

std::string escape_xml(std::string text)
{
    text = replace_all(std::move(text), "&", "&amp;");
    text = replace_all(std::move(text), "<", "&lt;");
    text = replace_all(std::move(text), ">", "&gt;");
    return text;
}

class template_fixture
{
    std::string name;
    std::string parameters{};
    std::string declarations{};

public:
    template_fixture(std::string name): name{std::move(name)} {}
    void set_name(std::string new_name) { name = std::move(new_name); }
    template_fixture& add_parameter(std::string param)
    {
        if (!parameters.empty())
            parameters += ", ";
        parameters += escape_xml(std::move(param));
        return *this;
    }
    template_fixture& add_declaration(std::string text)
    {
        if (!declarations.empty())
            declarations += '\n';
        declarations += escape_xml(std::move(text));
        return *this;
    }
    std::string str() const
    {
        static constexpr const char* simple_template = R"XML("<template>
        <name x="5" y="5">%s</name>
        <parameter>%s</parameter>
        <declaration>%s</declaration>
        <location id="id0" x="0" y="0"/>
        <init ref="id0"/>
    </template>")XML";
        return string_format(simple_template, name.c_str(), parameters.c_str(), declarations.c_str());
    }

private:
};

class QueryBuilder : public UTAP::StatementBuilder
{
    UTAP::Expression query;
    UTAP::TypeChecker checker;

public:
    explicit QueryBuilder(UTAP::Document& doc): UTAP::StatementBuilder{doc}, checker{doc}
    {
        // The document is already fully parsed, so builtin_decl_end() was never
        // called for this builder. Push the innermost declaration frame so
        // property expressions can resolve global and system declarations.
        push_frame(doc.get_system_declarations().frame);
    }
    void property() override
    {
        if (fragments.size() == 0)
            throw std::logic_error("No query fragments after building query");

        query = fragments[0];
        fragments.pop();
    }
    void strategy_declaration(std::string_view strategy_name) override {}
    void typecheck() { checker.checkExpression(query); }
    [[nodiscard]] UTAP::Expression getQuery() const { return query; }
    UTAP::Variable* add_variable(UTAP::Type type, std::string_view name, UTAP::Expression init,
                                UTAP::position_t pos) override
    {
        throw UTAP::NotSupportedException(__FUNCTION__);
    }
    bool add_function(UTAP::Type type, std::string_view name, UTAP::position_t pos) override
    {
        throw UTAP::NotSupportedException(__FUNCTION__);
    }
};

class QueryFixture
{
    UTAP::Document doc;
    UTAP::TigaPropertyBuilder query_builder;

public:
    QueryFixture(UTAP::Document&& doc): doc{std::move(doc)}, query_builder{this->doc} {}
    auto& get_errors() const { return doc.get_errors(); }
    const UTAP::PropInfo& parse_query(const char* query)
    {
        auto result = parse_property(query, query_builder);

        if (result == -1 || not doc.get_errors().empty()) {
            if (doc.get_errors().empty())
                throw std::logic_error("Query parsing failed with no errors");
            throw std::logic_error(doc.get_errors()[0].msg);
        }
        return query_builder.get_properties().back();
    }
};

class document_fixture
{
    std::string global_decls{};
    std::string templates{};
    std::string system_decls{};
    std::string processes{};

public:
    /** Adds text at the global declarations section of the document */
    document_fixture& add_global_decl(std::string text)
    {
        if (!global_decls.empty())
            global_decls += '\n';
        global_decls += escape_xml(std::move(text));
        return *this;
    }
    /** Adds a template text, @see template_fixture::str() */
    document_fixture& add_template(std::string text)
    {
        templates += '\n';
        templates += std::move(text);
        return *this;
    }
    /** Adds a default template and default process just like default initial document */
    document_fixture& add_default_process()
    {
        add_template(template_fixture("Template").str());
        add_system_decl("Process = Template();");
        add_process("Process");
        return *this;
    }
    /** Adds text to system declarations */
    document_fixture& add_system_decl(std::string text)
    {
        system_decls += '\n';
        system_decls += escape_xml(std::move(text));
        return *this;
    }
    /** Adds a process to the system */
    document_fixture& add_process(std::string name)
    {
        if (!processes.empty())
            processes += ", ";
        processes += std::move(name);
        return *this;
    }
    /** Compiles an XML string representation of a document */
    std::string str() const
    {
        static constexpr const char* document_template = R"XML(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE nta PUBLIC '-//Uppaal Team//DTD Flat System 1.6//EN' 'https://www.it.uu.se/research/group/darts/uppaal/flat-1_6.dtd'>
<nta>
    <declaration>%s</declaration>%s
    <system>%s
system %s;
    </system>
</nta>
)XML";
        return string_format(document_template, global_decls.c_str(), templates.c_str(), system_decls.c_str(),
                             processes.c_str());
    }

    /** Derives a document from a document template and parses it */
    [[nodiscard]] UTAP::Document parse() const
    {
        auto doc = UTAP::Document{};
        auto data = str();
        parse_XML_buffer(data.c_str(), doc, true);
        return doc;
    }

    [[nodiscard]] QueryFixture build_query_fixture() const { return QueryFixture(parse()); }
};

#endif  // INCLUDE_UTAP_DOCUMENT_FIXTURE_HPP
