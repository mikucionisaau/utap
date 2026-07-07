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

#include "utap/builder.hpp"

namespace UTAP {
TypeException unknown_identifier_error(std::string_view name)
{
    return TypeException{"$Unknown_identifier: " + std::string{name}};
}

TypeException has_no_such_member_error(std::string_view name)
{
    return TypeException{"$has_no_member_named " + std::string{name}};
}

TypeException is_not_a_struct_error(std::string_view name)
{
    return TypeException{std::string{name} + " $is_not_a_structure"};
}

TypeException duplicate_definition_error(std::string_view name)
{
    return TypeException{"$Duplicate_definition_of " + std::string{name}};
}

TypeException invalid_type_error(std::string_view name) { return TypeException{"$Invalid_type " + std::string{name}}; }

TypeException no_such_process_error(std::string_view name)
{
    return TypeException{"$No_such_process: " + std::string{name}};
}

TypeException not_a_template_error(std::string_view name)
{
    return TypeException{"$Not_a_template: " + std::string{name}};
}

TypeException not_a_process_error(std::string_view name)
{
    return TypeException{std::string{name} + " $is_not_a_process"};
}

TypeException strategy_not_declared_error(std::string_view name)
{
    return TypeException{"$strategy_not_declared: " + std::string{name}};
}

TypeException unknown_dynamic_template_error(std::string_view name)
{
    return TypeException{"Unknown dynamic template " + std::string{name}};
}

TypeException shadows_a_variable_warning(std::string_view name)
{
    return TypeException{std::string{name} + " $shadows_a_variable"};
}

}  // namespace UTAP
