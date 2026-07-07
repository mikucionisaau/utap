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

#include "utap/type.hpp"

#include "utap/common.hpp"
#include "utap/expression.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>  // std::pair

#include <cassert>
#include <cstdint>  // uint32_t

namespace UTAP {
using namespace Constants;

struct child_t
{
    std::string label;
    Type child;
    position_t position;
};

struct Type::type_data
{
    Kind kind;            // Kind of type object
    position_t position;  // Position in the input file
    Expression expr;      //
    std::vector<child_t> children;
    type_data(Kind kind, position_t position): kind{kind}, position{position} {}
};

Type::Type(Kind kind, const position_t& pos, size_t size)
{
    data = std::make_shared<type_data>(kind, pos);
    data->children.resize(size);
}

uint32_t Type::size() const
{
    assert(data);
    return static_cast<uint32_t>(data->children.size());
}

const Type& Type::operator[](uint32_t i) const
{
    assert(i < size());
    return data->children[i].child;
}

const Type& Type::get(uint32_t i) const
{
    assert(i < size());
    return data->children[i].child;
}

const std::string& Type::get_label(uint32_t i) const
{
    assert(i < size());
    return data->children[i].label;
}

const position_t& Type::get_field_position(uint32_t i) const
{
    assert(i < size());
    return data->children[i].position;
}

std::optional<uint32_t> Type::find_index_of(std::string_view label) const
{
    assert(is_record() || is_process());
    const Type type = strip();
    const auto n = type.size();
    for (auto i = 0u; i < n; ++i) {
        if (type.get_label(i) == label) {
            return i;
        }
    }
    return {};
}

Kind Type::get_kind() const { return unknown() ? UNKNOWN : data->kind; }

bool Type::is_prefix() const
{
    switch (get_kind()) {
    case Constants::FRACTION:
    case Constants::UNKNOWN:
    case Constants::VOID_TYPE:
    case Constants::CLOCK:
    case Constants::INT:
    case Constants::DOUBLE:
    case Constants::BOOL:
    case Constants::STRING:
    case Constants::SCALAR:
    case Constants::LOCATION:
    case Constants::LOCATION_EXPR:
    case Constants::BRANCHPOINT:
    case Constants::CHANNEL:
    case Constants::COST:
    case Constants::INVARIANT:
    case Constants::INVARIANT_WR:
    case Constants::GUARD:
    case Constants::DIFF:
    case Constants::CONSTRAINT:
    case Constants::FORMULA:
    case Constants::ARRAY:
    case Constants::RECORD:
    case Constants::PROCESS:
    case Constants::PROCESS_SET:
    case Constants::FUNCTION:
    case Constants::FUNCTION_EXTERNAL:
    case Constants::INSTANCE:
    case Constants::RANGE:
    case Constants::REF:
    case Constants::TYPEDEF:
    case Constants::LABEL:
    case Constants::RATE:
    case Constants::INSTANCE_LINE:  // LSC
    case Constants::MESSAGE:        // LSC
    case Constants::CONDITION:      // LSC
    case Constants::UPDATE:         // LSC
    case Constants::LSC_INSTANCE:   // LSC
        return false;

    default: return true;
    }
}

bool Type::unknown() const { return data == nullptr || data->kind == UNKNOWN; }

bool Type::is(Kind kind) const
{
    using namespace Constants;
    const auto k = get_kind();
    if (k == PROCESS_VAR) {
        return kind == PROCESS_VAR;
    }
    if (k == DOUBLE_INV_GUARD) {
        return kind == DOUBLE_INV_GUARD;
    }
    return (k == kind) || (is_prefix() && get(0).is(kind)) || (k == RANGE && get(0).is(kind)) ||
           (k == REF && get(0).is(kind)) || (k == LABEL && get(0).is(kind));
}

Type Type::get_sub() const
{
    assert(is_array());
    const auto k = get_kind();
    if (k == REF || get_kind() == LABEL) {
        return get(0).get_sub();
    } else if (is_prefix()) {
        return get(0).get_sub().create_prefix(k);
    } else {
        return get(0);
    }
}

Type Type::get_sub(uint32_t i) const
{
    assert(is_record() || is_process());
    const auto k = get_kind();
    if (k == REF || k == LABEL) {
        return get(0).get_sub(i);
    } else if (is_prefix()) {
        return get(0).get_sub(i).create_prefix(k);
    } else {
        return get(i);
    }
}

const Type& Type::get_array_size() const
{
    const auto k = get_kind();
    if (is_prefix() || k == REF || k == LABEL) {
        return get(0).get_array_size();
    } else {
        assert(k == ARRAY);
        return get(1);
    }
}

uint32_t Type::get_record_size() const
{
    if (const auto k = get_kind(); is_prefix() || k == REF || k == LABEL) {
        return get(0).get_record_size();
    } else {
        assert(k == RECORD);
        return size();
    }
}

const std::string& Type::get_record_label(uint32_t i) const
{
    static const auto location = std::string{"location"};
    if (const auto k = get_kind(); is_prefix() || k == REF || k == LABEL) {
        return get(0).get_record_label(i);
    } else if (i == static_cast<size_t>(std::numeric_limits<int32_t>::max())) {
        // TODO: create a separate type for location expressions
        // TODO: get rid of magical constants
        return location;
    } else {
        assert(k == RECORD || k == PROCESS);
        return get_label(i);
    }
}

std::pair<Expression, Expression> Type::get_range() const
{
    assert(is(RANGE));
    if (get_kind() == RANGE) {
        return std::make_pair(get(1).get_expression(), get(2).get_expression());
    } else {
        return get(0).get_range();
    }
}

const Expression& Type::get_expression() const
{
    assert(data);
    return data->expr;
}

Type Type::strip() const
{
    const auto k = get_kind();
    if (is_prefix() || k == RANGE || k == REF || k == LABEL) {
        return get(0).strip();
    } else {
        return *this;
    }
}

Type Type::strip_array() const
{
    Type type = strip();
    while (type.get_kind() == ARRAY) {
        type = type.get(0).strip();
    }
    return type;
}

Type Type::rename(const std::string& from, const std::string& to) const
{
    auto type = Type{get_kind(), get_position(), size()};
    type.data->expr = get_expression();
    for (auto i = 0u; i < size(); ++i) {
        type.data->children[i].child = get(i).rename(from, to);
        type.data->children[i].label = get_label(i);
    }
    if (get_kind() == LABEL && get_label(0) == from) {
        type.data->children[0].label = to;
    }
    return type;
}

Type Type::subst(const Symbol& symbol, const Expression& expr) const
{
    auto type = Type{get_kind(), get_position(), size()};
    for (auto i = 0u; i < size(); i++) {
        type.data->children[i].label = get_label(i);
        type.data->children[i].child = get(i).subst(symbol, expr);
    }
    if (!data->expr.empty()) {
        type.data->expr = data->expr.subst(symbol, expr);
    }
    return type;
}

const position_t& Type::get_position() const { return data->position; }

bool Type::is_constant() const
{
    switch (get_kind()) {
    case FUNCTION:
    case FUNCTION_EXTERNAL:
    case PROCESS:
    case INSTANCE:
    case LSC_INSTANCE: return false;
    case CONSTANT: return true;
    case RECORD:
        return std::all_of(data->children.begin(), data->children.end(),
                           [](const child_t& c) { return c.child.is_constant(); });
    default: return size() > 0 && get(0).is_constant();
    }
}

bool Type::is_mutable() const
{
    switch (get_kind()) {
    case FUNCTION:
    case FUNCTION_EXTERNAL:
    case PROCESS:
    case INSTANCE:
    case LSC_INSTANCE:
    case CONSTANT: return false;
    case RECORD:
        return std::all_of(data->children.begin(), data->children.end(),
                           [](const child_t& c) { return c.child.is_mutable(); });
    default: return size() == 0 || get(0).is_mutable();
    }
}

bool Type::is_equality_compatible(const Type& other) const
{
    if (is_integral() && other.is_integral()) {
        return true;
    } else if (is(PROCESS_VAR) && other.is(PROCESS_VAR)) {
        return true;
    } else {
        return is_equivalent(other);
    }
}

bool Type::is_assignment_compatible(const Type& rhs, bool init) const
{
    if (init ? ((is_clock() && (rhs.is_double() || rhs.is_integral())) ||
                (is_double() && (rhs.is_double() || rhs.is_integral())))
             : ((is_clock() || is_double()) && (rhs.is_integral() || rhs.is_double() || rhs.is_clock()))) {
        return true;
             } else if (is_integral() && rhs.is_integral()) {
                 return true;
             }
    return is_equivalent(rhs);
}

bool Type::is_inline_if_compatible(const Type& t1, const Type& t2) const
{
    if (is_assignment_compatible(t1) && is_assignment_compatible(t2))
        return true;
    return t1.is_equivalent(t2);
}

int Type::channel_capability() const
{
    assert(is_channel());
    if (is(URGENT))
        return 0;
    if (is(BROADCAST))
        return 1;
    return 2;
}

/**
 * Returns true if two scalar types are name-equivalent.
 */
bool Type::is_same_scalar(const Type& o) const
{
    if (get_kind() == REF || get_kind() == CONSTANT || get_kind() == SYSTEM_META) {
        return (*this)[0].is_same_scalar(o);
    } else if (o.get_kind() == EF || o.get_kind() == CONSTANT || o.get_kind() == SYSTEM_META) {
        return is_same_scalar(o[0]);
    } else if (get_kind() == LABEL && o.get_kind() == LABEL) {
        return get_label(0) == o.get_label(0) && (*this)[0].is_same_scalar(o[0]);
    } else if (get_kind() == SCALAR && o.get_kind() == SCALAR) {
        return true;
    } else if (get_kind() == RANGE && o.get_kind() == RANGE) {
        return (*this)[0].is_same_scalar(o[0]) && get_range().first.equal(o.get_range().first) &&
               get_range().second.equal(o.get_range().second);
    } else {
        return false;
    }
}

bool Type::is_equivalent(const Type& o) const
{
    if (is_integer() && o.is_integer()) {
        return !is(RANGE) || !o.is(RANGE) ||
               (get_range().first.equal(o.get_range().first) && get_range().second.equal(o.get_range().second));
    } else if (is_clock() && o.is_clock()) {
        return true;
    } else if (is_scalar() && o.is_scalar()) {
        return is_same_scalar(o);
    } else if (is_double() && o.is_double()) {
        return true;
    } else if (is_boolean() && o.is_boolean()) {
        return true;
    } else if (is_channel() && o.is_channel()) {
        return channel_capability() == o.channel_capability();
    } else if (is_record() && o.is_record()) {
        const auto size = get_record_size();
        if (const auto oSize = o.get_record_size(); size == oSize) {
            for (uint32_t i = 0; i < size; ++i) {
                if (get_record_label(i) != o.get_record_label(i) || !get_sub(i).is_equivalent(o.get_sub(i))) {
                    return false;
                }
            }
            return true;
        }
    } else if (is_array() && o.is_array()) {
        const Type& size = get_array_size();
        if (const Type& oSize = o.get_array_size(); size.is_integer() && oSize.is_integer()) {
            return size.get_range().first.equal(oSize.get_range().first) &&
                   size.get_range().second.equal(oSize.get_range().second) && get_sub().is_equivalent(o.get_sub());
        } else if (size.is_scalar() && oSize.is_scalar()) {
            return size.is_same_scalar(oSize) && get_sub().is_equivalent(o.get_sub());
        }
        return false;
    } else if (is_string() && o.is_string()) {
        return true;
    }
    return false;
}

Type Type::create_range(Type type, Expression lower, Expression upper, position_t pos)
{
    auto t = Type{RANGE, pos, 3};
    t.data->children[0].child = std::move(type);
    t.data->children[1].child = Type{UNKNOWN, pos, 0};
    t.data->children[2].child = Type{UNKNOWN, pos, 0};
    t[1].data->expr = std::move(lower);
    t[2].data->expr = std::move(upper);
    return t;
}

Type Type::create_record(const std::vector<Type>& types, const std::vector<std::string>& labels, const std::vector<position_t>& positions, position_t pos)
{
    assert(types.size() == labels.size());
    auto type = Type{RECORD, pos, types.size()};
    for (size_t i = 0; i < types.size(); i++) {
        type.data->children[i].child = types[i];
        type.data->children[i].label = labels[i];
        type.data->children[i].position = positions[i];
    }
    return type;
}

Type Type::create_function(Type ret, const std::vector<Type>& parameters, const std::vector<std::string>& labels,
                           position_t pos)
{
    assert(parameters.size() == labels.size());
    auto type = Type{FUNCTION, pos, parameters.size() + 1};
    type.data->children[0].child = std::move(ret);
    for (size_t i = 0; i < parameters.size(); i++) {
        type.data->children[i + 1].child = parameters[i];
        type.data->children[i + 1].label = labels[i];
    }
    return type;
}

Type Type::create_external_function(Type ret, const std::vector<Type>& parameters,
                                    const std::vector<std::string>& labels, position_t pos)
{
    assert(parameters.size() == labels.size());
    auto type = Type{FUNCTION_EXTERNAL, pos, parameters.size() + 1};
    type.data->children[0].child = std::move(ret);
    for (size_t i = 0; i < parameters.size(); i++) {
        type.data->children[i + 1].child = parameters[i];
        type.data->children[i + 1].label = labels[i];
    }
    return type;
}

Type Type::create_array(Type sub, Type size, position_t pos)
{
    auto type = Type{ARRAY, pos, 2};
    type.data->children[0].child = std::move(sub);
    type.data->children[1].child = std::move(size);
    return type;
}

Type Type::create_typedef(std::string label, Type type, position_t pos)
{
    auto t = Type{TYPEDEF, pos, 1};
    t.data->children[0].label = std::move(label);
    t.data->children[0].child = std::move(type);
    return t;
}

Type Type::create_instance(const Frame& parameters, position_t pos)
{
    auto type = Type{INSTANCE, pos, parameters.get_size()};
    for (auto i = 0u; i < parameters.get_size(); ++i) {
        type.data->children[i].child = parameters[i].get_type();
        type.data->children[i].label = parameters[i].get_name();
    }
    return type;
}

Type Type::create_LSC_instance(const Frame& parameters, position_t pos)
{
    auto type = Type{LSC_INSTANCE, pos, parameters.get_size()};
    for (auto i = 0u; i < parameters.get_size(); ++i) {
        type.data->children[i].child = parameters[i].get_type();
        type.data->children[i].label = parameters[i].get_name();
    }
    return type;
}

Type Type::create_process(const Frame& frame, position_t pos)
{
    auto type = Type{PROCESS, pos, frame.get_size()};
    for (auto i = 0u; i < frame.get_size(); ++i) {
        type.data->children[i].child = frame[i].get_type();
        type.data->children[i].label = frame[i].get_name();
    }
    return type;
}

Type Type::create_process_set(const Type& instance, position_t pos)
{
    auto type = Type{PROCESS_SET, pos, instance.size()};
    for (auto i = 0u; i < instance.size(); ++i) {
        type.data->children[i].child = instance[i];
        type.data->children[i].label = instance.get_label(i);
    }
    return type;
}

Type Type::create_primitive(Kind kind, position_t pos) { return Type(kind, pos, 0); }

Type Type::create_prefix(Kind kind, position_t pos) const
{
    auto type = Type{kind, pos, 1};
    type.data->children[0].child = *this;
    return type;
}

Type Type::create_label(std::string_view label, position_t pos) const
{
    auto type = Type{LABEL, pos, 1};
    type.data->children[0].child = *this;
    type.data->children[0].label = label;
    return type;
}

std::ostream& Type::print(std::ostream& os) const
{
    if (data == nullptr)
        return os << "unknown";

    if (!data->expr.empty())
        return data->expr.print(os << '"') << '"';

    os << "(";
    switch (get_kind()) {
    case UNKNOWN: os << "unknown"; break;
    case RANGE: os << "range"; break;
    case ARRAY: os << "array"; break;
    case RECORD: os << "struct"; break;
    case CONSTANT: os << "const"; break;
    case REF: os << "ref"; break;
    case URGENT: os << "urgent"; break;
    case COMMITTED: os << "committed"; break;
    case BROADCAST: os << "broadcast"; break;
    case VOID_TYPE: os << "void"; break;
    case CLOCK: os << "clock"; break;
    case FRACTION: os << "fraction"; break;
    case INT: os << "int"; break;
    case DOUBLE: os << "double"; break;
    case STRING: os << "string"; break;
    case BOOL: os << "bool"; break;
    case SCALAR: os << "scalar"; break;
    case CHANNEL: os << "channel"; break;
    case INVARIANT: os << "invariant"; break;
    case GUARD: os << "guard"; break;
    case DIFF: os << "diff"; break;
    case CONSTRAINT: os << "constraint"; break;
    case FORMULA: os << "formula"; break;
    case COST: os << "cost"; break;
    case RATE: os << "rate"; break;
    case TYPEDEF: os << "def"; break;
    case PROCESS: os << "process"; break;
    case INSTANCE: os << "instance"; break;
    case LABEL: os << "label"; break;
    case FUNCTION_EXTERNAL: os << "external_function"; break;
    case FUNCTION: os << "function"; break;
    case LOCATION_EXPR:
    case LOCATION: os << "location"; break;
    case BRANCHPOINT: os << "branchpoint"; break;
        // LSC
    case INSTANCE_LINE: os << "instance line"; break;
    case MESSAGE: os << "message"; break;
    case CONDITION: os << "condition"; break;
    case UPDATE: os << "update"; break;
    case LSC_INSTANCE: os << "LSC_instance"; break;
    case PROCESS_VAR: os << "process_var"; break;
    default: os << "type(" + std::to_string(get_kind()) + ")"; break;
    }

    for (uint32_t i = 0; i < size(); i++) {
        os << " ";
        if (!get_label(i).empty())
            os << get_label(i) << ':';
        get(i).print(os);
    }
    return os << ')';
}

std::ostream& Type::print_declaration(std::ostream& os) const
{
    if (data == nullptr)
        os << "unknown";

    if (!data->expr.empty())
        return os;

    auto kind = std::string{};
    auto range = false;
    auto array = false;
    auto label = false;
    auto typeDef = false;

    switch (get_kind()) {
    case UNKNOWN: kind = "unknown"; break;
    case RANGE: range = true; break;
    case ARRAY: array = true; break;
    case RECORD: kind = "struct"; break;
    case CONSTANT: kind = "const"; break;
    case REF: kind = "ref"; break;
    case URGENT: kind = "urgent"; break;
    case COMMITTED: kind = "committed"; break;
    case BROADCAST: kind = "broadcast"; break;
    case VOID_TYPE: kind = "void"; break;
    case CLOCK: kind = "clock"; break;
    case INT: kind = "int"; break;
    case DOUBLE: kind = "double"; break;
    case STRING: kind = "string"; break;
    case BOOL: kind = "bool"; break;
    case SCALAR: kind = "scalar"; break;
    case CHANNEL: kind = "chan"; break;
    case INVARIANT: kind = "invariant"; break;
    case GUARD: kind = "guard"; break;
    case DIFF: kind = "diff"; break;
    case CONSTRAINT: kind = "constraint"; break;
    case FORMULA: kind = "formula"; break;
    case COST: kind = "cost"; break;
    case RATE: kind = "rate"; break;
    case TYPEDEF:
        kind = "typedef";
        typeDef = true;
        break;
    case PROCESS: kind = "process"; break;
    case INSTANCE: kind = "instance"; break;
    case LABEL: label = true; break;
    case FUNCTION_EXTERNAL: kind = "external_function"; break;
    case FUNCTION: kind = "function"; break;
    default: kind = "type(" + std::to_string(get_kind()) + ")"; break;
    }

    if (range) {
        get(0).print_declaration(os);
        if (get_range().first.get_value() != INT16_MIN || get_range().second.get_value() != INT16_MAX) {
            os << "[";
            get_range().first.print(os) << ",";
            get_range().second.print(os) << "]";
            get(1).print_declaration(os);
        }
    } else if (array) {
        get(0).print_declaration(os) << '[';
        get_array_size().get_range().second.get(0).print(os) << ']';
    } else if (label) {
        os << get_label(0);
    } else if (typeDef) {
        os << kind << " ";
        get(0).print_declaration(os) << ' ' << get_label(0);
    } else {
        os << kind;
        for (uint32_t i = 0; i < size(); i++) {
            os << ' ';
            if (!get_label(i).empty())
                os << get_label(i) << ":";
            get(i).print_declaration(os);
        }
    }
    return os;
}

std::string Type::str() const
{
    auto os = std::ostringstream{};
    print(os);
    return os.str();
}

std::string Type::declaration() const
{
    auto os = std::ostringstream{};
    print_declaration(os);
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Type& t) { return t.print(os); }

} // namespace UTAP