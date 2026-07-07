/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020 Aalborg University.
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

#ifndef UTAP_STRINGINTERNING_H
#define UTAP_STRINGINTERNING_H

#include <algorithm>
#include <string>
#include <vector>

namespace UTAP {

/**
 * Reuses strings to reduce memory footprint
 */
class InternedStrings
{
public:
    class Index
    {
        const InternedStrings* interned;  // pointer allows default copy operations
        size_t id;
        Index(const InternedStrings& interned, size_t id): interned{&interned}, id{id} {}
        friend class InternedStrings;

    public:
        size_t index() const { return id; }
        const std::string& str() const { return interned->strings[id]; }
        bool operator==(const Index& other) const { return interned == other.interned && id == other.id; }
    };
    const std::vector<std::string>& get_strings() const { return strings; }
    Index add_string_if_new(std::string&& string)
    {
        auto it = std::find(std::begin(strings), std::end(strings), string);
        if (it == std::end(strings)) {
            strings.push_back(std::move(string));
            return Index{*this, strings.size() - 1};
        } else {
            return Index{*this, static_cast<size_t>(it - std::begin(strings))};
        }
    }

private:
    std::vector<std::string> strings;
};

using StringIndex = InternedStrings::Index;

}  // namespace UTAP

#endif /* UTAP_STRINGINTERNING_H */