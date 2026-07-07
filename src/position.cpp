// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020 Aalborg University.
   Copyright (C) 2006 Uppsala University and Aalborg University.

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

#include "utap/position.hpp"

#include <iostream>
#include <stdexcept>

using std::string;

namespace UTAP {

void PositionIndex::add(uint32_t position, uint32_t offset, uint32_t line, std::shared_ptr<string> path)
{
    if (!lines.empty() && position < lines.back().position) {
        throw std::logic_error{"Positions must be monotonically increasing"};
    }
    lines.emplace_back(position, offset, line, std::move(path));
}

uint32_t PositionIndex::find_index(uint32_t position) const
{
    auto first = uint32_t{0};
    auto last = static_cast<uint32_t>(lines.size());
    while (first + 1 < last) {
        uint32_t i = (first + last) / 2;
        if (position < lines[i].position) {
            last = i;
        } else {
            first = i;
        }
    }
    return first;
}

const PositionIndex::Line& PositionIndex::find(uint32_t position) const
{
    if (lines.empty())
        throw std::logic_error{"No positions have been added"};
    return lines[find_index(position)];
}

const PositionIndex::Line& PositionIndex::find_first_line(uint32_t position) const
{
    if (lines.empty()) {
        throw std::logic_error{"No positions have been added"};
    }
    int index = find_index(position);
    while (lines[index].line > 1)
        index--;

    return lines[index];
}

/** Dump table to stdout. */
std::ostream& PositionIndex::print(std::ostream& os) const
{
    for (const auto& line : lines)
        os << line.position << " " << line.offset << " " << line.line << " " << line.path << std::endl;
    return os;
}

std::ostream& operator<<(std::ostream& os, const Error& e)
{
    if (!e.start.path || e.start.path->empty()) {
        os << e.msg << " at line " << e.start.line << " column " << (e.position.start - e.start.position) << " to line "
           << e.end.line << " column " << (e.position.end - e.end.position);
    } else {
        os << e.msg << " in " << e.start.path << " at line " << e.start.line << " column "
           << (e.position.start - e.start.position) << " to line " << e.end.line << " column "
           << (e.position.end - e.end.position);
    }
    return os;
}

std::string Error::str() const
{
    if (position.start < start.position || position.end < end.position)
        return msg + " (Unknown position in document)";
    if (!start.path || start.path->empty()) {
        return msg + " at line " + std::to_string(start.line) + " column " +
               std::to_string(position.start - start.position) + " to line " + std::to_string(end.line) + " column " +
               std::to_string(position.end - end.position);
    } else {
        return msg + " in " + *start.path + " at line " + std::to_string(start.line) + " column " +
               std::to_string(position.start - start.position) + " to line " + std::to_string(end.line) + " column " +
               std::to_string(position.end - end.position);
    }
}

}  // namespace UTAP
