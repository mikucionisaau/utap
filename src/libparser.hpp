// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020 Aalborg University.
   Copyright (C) 2002 Uppsala University and Aalborg University.

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

#ifndef UTAP_LIBPARSER_HH
#define UTAP_LIBPARSER_HH

#include "utap/builder.hpp"

#include <memory>
#include <system_error>

// The maximum length is 4000 (see error message) + 1 for the
// terminating \0.
constexpr auto MAXLEN = 4001u;
constexpr auto ID_TOO_LONG = "$Identifier_is_too_long._Limit_length_is_4000.";

enum class Syntax : unsigned int {
    NONE = 0u,
    OLD = (1u << 0),
    NEW = (1u << 1),
    PROPERTY = (1u << 2),
    GUIDING = (1u << 3),
    TIGA = (1u << 4),
    PROB = (1u << 5),
    OLD_NEW = OLD | NEW,
    NEW_PROPERTY = NEW | PROPERTY,
    OLD_PROPERTY = OLD | PROPERTY,
    OLD_NEW_PROPERTY = OLD | NEW_PROPERTY,
    PROPERTY_TIGA = PROPERTY | TIGA,
    PROPERTY_PROB = PROPERTY | PROB,
    NEW_GUIDING = NEW | GUIDING,
    OLD_GUIDING = OLD | GUIDING
};

constexpr Syntax operator&(const Syntax& s1, const Syntax& s2)
{
    using T = std::underlying_type_t<Syntax>;
    return Syntax{static_cast<T>(s1) & static_cast<T>(s2)};
}

namespace UTAP {
/**
 * Help class used by the lexer, parser and xmlreader to keep
 * track of the current position.
 */
struct PositionTracker
{
    uint32_t line{};
    uint32_t offset{};
    uint32_t position{};
    std::shared_ptr<std::string> path;

    /**
     * Sets the current path to \a s, offset to 0 and line to 1.
     * Sets the position of \a builder to [position, position + 1)
     * (a one character dummy position; this is useful when
     * assigning error messages to XML elements without a text
     * content). Adds position to \a builder and increments it by
     * 1.
     */
    void setPath(UTAP::ParserBuilder* parser, std::string_view s)
    {
        // Incrementing the position by one avoids the problem where the
        // end-position happens to bleed into a path. E.g. the range 5-10
        // contains 5 character (at positions 5, 6, 7, 8 and 9), thus
        // position 10 could have a new path). An alternative would be to
        // subtract 1 before calling Positions::find().
        line = 1;
        offset = 0;
        path = std::make_shared<std::string>(s);
        ++position;
        parser->add_position(position, offset, line, path);
    }

    /**
     * This overload provides a way reuse paths over multiple setPath calls
     */
    void setPath(UTAP::ParserBuilder* parser, std::shared_ptr<std::string> s)
    {
        line = 1;
        offset = 0;
        path = std::move(s);
        ++position;
        parser->add_position(position, offset, line, path);
    }

    /**
     * Sets the position of \a builder to [position, position + n)
     * and increments position and offset by \a n.
     */
    int increment(UTAP::ParserBuilder* parser, uint32_t n)
    {
        parser->set_position(position, position + n);
        position += n;
        offset += n;
        return static_cast<int>(position - n);
    }

    /**
     * Increments line by \a n and adds the position to \a
     * builder.
     */
    void newline(UTAP::ParserBuilder* parser, uint32_t n)
    {
        line += n;
        parser->add_position(position, offset, line, path);
    }
};

extern PositionTracker tracker;  // defined in lexer.l

/** Errors from underlying XML reading operations (most likely OS issues) */
class XMLReaderError : public std::system_error
{
public:
    using std::system_error::system_error;
};
/** Errors due to wrong XML document structure */
class XMLDocError : public std::logic_error
{
public:
    using std::logic_error::logic_error;
};
}  // namespace UTAP

#endif /* UTAP_LIBPARSER_HH */
