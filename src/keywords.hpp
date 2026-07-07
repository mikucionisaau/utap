// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2022 Aalborg University.

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

#ifndef INCLUDE_UTAP_KEYWORDS_HPP
#define INCLUDE_UTAP_KEYWORDS_HPP

#include "libparser.hpp"

#include <string_view>
#include <cinttypes>  // uint32_t

namespace UTAP {
struct Keyword
{
    int token{};
    Syntax syntax{};
};

/**
 * Checks if the text is a keyword based on the specified syntax.
 * @param text the string
 * @param syntax the syntax
 * @return true if the text is a keyword according to the specified syntax.
 */
bool is_keyword(std::string_view word, Syntax syntax);

/**
 * Searches for the text among the keywords and returns address if found.
 * @param text the string to search for
 * @return the keyword struct if found and nullptr otherwise
 */
const Keyword* find_keyword(std::string_view word);

}  // namespace UTAP

#endif /* INCLUDE_UTAP_KEYWORDS_HPP */
