// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

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

#ifndef UTAP_TYPE_HH
#define UTAP_TYPE_HH

#include "utap/common.hpp"
#include "utap/position.hpp"

#include <memory>  // shared_ptr
#include <optional>
#include <string>
#include <cstdint>

namespace UTAP {
class Expression;
class Frame;
class Symbol;

/**
   A reference to a type.

   Types are represented as trees of types. The type cannot be
   access directly. You need to use an instance of Type to
   access a type. Internally, types are reference counted and do
   not need to be deallocated manually. Types are immutable.

   Types are either primitive such as clocks or channels, or
   contructed types such as structs and array. Constructed types
   are created using one of the factory methods in the Type
   class. Primitive types are leaves in the tree, constructed
   types are inner nodes.

   All types have a kind - a type for the type. The kind is a
   value of Kind. In anticipation of a future homogeneous AST,
   this kind is defined in the same enumerable as the kind of
   expressions.

   Some constructed types are considered prefixes for other types:
   URGENT, COMMITTED, BROADCAST, CONSTANT and BROADCAST.

   LABEL types are references to named types. These are introduced
   by references to types defined with a typedef or by scalar set
   declarations. They have one child: The type which has been
   named.

   Constructed types are:

   - ARRAY; the first child is the element type, the second type
     is the array size (another type; either integer or scalarset).

   - RECORD; the children are the fields of the record.

   - FUNCTION; the first child is the return type. Remaining
     children are parameters.

   - INSTANCE; the parameters of an instance are the children.

   - PROCESS; the local variables of a process are the children.

   - PROCESSSET; process with unbound parameters. The unbound
     parameters are the children.

   - TYPEDEF; a type definition - it has one child.

   - RANGE; ranges have three children: the first is the type on
     which a range has been applied, the second is the lower bound
     and the third is the upper bound (the second and third
     children are really expression objects wrapped in a dummy
     type).

   - REF; a reference - the first child is the type from which the
     reference type is formed.
*/
class Type
{
    struct type_data;
    std::shared_ptr<type_data> data;

public:
    explicit Type(Constants::Kind kind, const position_t& pos, size_t size);
    /// Default constructor creates a null-type.
    Type() = default;

    /// Checks if types are equal
    bool operator==(const Type& other) const { return data == other.data; }

    /// Checks if types are not equal
    bool operator!=(const Type& other) const { return !(*this == other); }

    /// Returns the kind of type object.
    Constants::Kind get_kind() const;

    /**
     * Returns the position of the type in the input file. This
     * exposes the fact that the type is actually part of the AST.
     */
    const position_t& get_position() const;

    /// Returns the number of children.
    uint32_t size() const;

    /// Less-than operator.
    bool operator<(const Type& other) const { return data < other.data; }

    /// Returns the \a i'th child.
    const Type& operator[](uint32_t) const;

    /// Returns the \a i'th child.
    const Type& get(uint32_t) const;

    /// Returns the \a i'th label.
    const std::string& get_label(uint32_t) const;

    /** Returns the position of i'th field in struct */
    const position_t& get_field_position(uint32_t) const;

    /// Returns the expression associated with the type.
    const Expression& get_expression() const;

    /// Returns the size of an array (this is itself a type). @pre is_array().
    const Type& get_array_size() const;

    /// Returns the element type of an array. Preserves any prefixes. @pre is_array();
    Type get_sub() const;

    /**
     * Returns the \i'th field of a record or process. Preserves
     * any prefixes. @pre is_record() or is_process().
     */
    Type get_sub(uint32_t) const;

    /// Returns the number of fields of a record. @pre is_record().
    uint32_t get_record_size() const;

    /// Returns the label of the \i'th field of a record. @pre is_record().
    const std::string& get_record_label(uint32_t i) const;

    /**
     * Returns the index of the record or process field with the
     * given label. Returns -1 if such a field does not
     * exist. @pre is_record() or is_process().
     */
    std::optional<uint32_t> find_index_of(std::string_view) const;

    /// Returns the range of a RANGE type. @pre is_range().
    std::pair<Expression, Expression> get_range() const;

    /// Prints textual representation of the type.
    std::ostream& print(std::ostream&) const;

    /// Generates string representation of the type.
    std::string str() const;

    /// Pretty-prints the declaration.
    std::ostream& print_declaration(std::ostream& os) const;

    /// Generates the declaration representation.
    std::string declaration() const;

    /// Shortcut for is(RANGE).
    bool is_range() const { return is(Constants::RANGE); }

    /// Shortcut for is(INT).
    bool is_integer() const { return is(Constants::INT); }

    /// Shortcut for is(BOOL).
    bool is_boolean() const { return is(Constants::BOOL); }

    /// Shortcut for is(FUNCTION).
    bool is_function() const { return is(Constants::FUNCTION); }

    /// Shortcut for is(FUNCTION_EXTERNAL).
    bool is_function_external() const { return is(Constants::FUNCTION_EXTERNAL); }

    /// Shortcut for is(PROCESS).
    bool is_process() const { return is(Constants::PROCESS); }

    /// Shortcut for is(PROCESS_SET).
    bool is_process_set() const { return is(Constants::PROCESS_SET); }

    /// Shortcut for is(LOCATION).
    bool is_location() const { return is(Constants::LOCATION); }

    /// Shortcut for is(LOCATION_EXPR).
    bool is_location_expr() const { return is(Constants::LOCATION_EXPR); }

    /// Shortcut for is(INSTANCELINE).
    bool is_instance_line() const { return is(Constants::INSTANCE_LINE); }

    /// Shortcut for is(BRANCHPOINT).
    bool is_branchpoint() const { return is(Constants::BRANCHPOINT); }

    /// Shortcut for is(CHANNEL).
    bool is_channel() const { return is(Constants::CHANNEL); }

    /// Shortcut for is(ARRAY).
    bool is_array() const { return is(Constants::ARRAY); }

    /// Shortcut for is(SCALAR).
    bool is_scalar() const { return is(Constants::SCALAR); }

    /// Shortcut for is(CLOCK).
    bool is_clock() const { return is(Constants::CLOCK); }

    /// Shortcut for is(RECORD).
    bool is_record() const { return is(Constants::RECORD); }

    /// Shortcut for is(DIFF).
    bool is_diff() const { return is(Constants::DIFF); }

    /// Shortcut for is(VOID_TYPE).
    bool is_void() const { return is(Constants::VOID_TYPE); }

    /// Shortcut for is(COST).
    bool is_cost() const { return is(Constants::COST); }

    /// Shortcut for is(DOUBLE).
    bool is_double() const { return is(Constants::DOUBLE); }

    /// Shortcut for is(STRING).
    bool is_string() const { return is(Constants::STRING); }

    /// True if the type is a boolean, integer, or location
    bool is_integral() const
    {
        using namespace Constants;
        return is(INT) || is(BOOL) || is(PROCESS_VAR) || is(LOCATION) || is(LOCATION_EXPR);
    }

    /**
     * Returns true if this is an invariant, boolean or
     * integer. Shortcut for isIntegral() || is(INVARIANT).
     */
    bool is_invariant() const { return is(Constants::INVARIANT) || is_integral(); }

    /**
     * Returns true if this is a guard, invariant, boolean or
     * integer.  Shortcut for is(GUARD) || is_invariant().
     */
    bool is_guard() const { return is(Constants::GUARD) || is_invariant(); }

    /**
     * Returns true if this is a probability or integer.  Shortcut
     * for is(PROBABILITY) || isInteger().
     */
    bool is_probability() const { return is(Constants::PROBABILITY) || is_integer() || is_double() || is_clock(); }

    /**
     * Returns true if this is a constraint, guard, invariant,
     * boolean or integer. Shortcut for is(CONSTRAINT) ||
     * is_guard().
     */
    bool is_constraint() const { return is(Constants::CONSTRAINT) || is_guard(); }

    /**
     * Returns true if this is a formula, constraint, guard,
     * invariant, boolean or integer. Shortcut for is(FORMULA) ||
     * is_constraint().
     */
    bool is_formula() const { return is(Constants::FORMULA) || is_constraint(); }

    /**
     * Removes any leading prefixes, RANGE, REF and LABEL types
     * and returns the result.
     */
    Type strip() const;

    /**
     * Removes any leading prefixes, RANGE, REF, LABEL and ARRAY
     * types and returns the result.
     */
    Type strip_array() const;

    /**
     * Returns false for non-prefix types and true
     * otherwise. Non-prefix types are PRIMITIVE, ARRAY, RECORD,
     * PROCESS, TEMPLATE, FUNCTION, INSTANCE, LSCINSTANCE, RANGE, REF, TYPEDEF.
     */
    bool is_prefix() const;

    /// Returns true if and only if all elements of the type are constant.
    bool is_constant() const;

    /// Returns true if and only if all elements of the type are mutable (not constant).
    bool is_mutable() const;

    /** Returns true if the type has kind \a kind or if type is a
     * prefix, RANGE or REF type and the getChild().is(kind)
     * returns true. */
    bool is(Constants::Kind kind) const;

    /** Returns true if two types are compatible for equality operator.
     * Types are compatible if they are structurally
     * equivalent. However for scalar we use name equivalence.  Prefixes
     * like CONST, SYSTEM_META, URGENT, COMMITTED, BROADCAST, REF and TYPENAME
     * are ignored.
     * Clocks are not handled by this method: If t1 or t2 are clock-types,
     * then false is returned.
     * TODO: REVISIT: This should be updated. */
    bool is_equality_compatible(const Type& other) const;

    /** Returns true if this type and rhs are assignment compatible.
     * This is the case when an expression of type rvalue can be assigned to
     * this type. It does not check whether this is actually a left-hand side value.
     * In case of integers, it does not check the range of the expressions. */
    bool is_assignment_compatible(const Type& rhs, bool init = false) const;

    /** Returns true if arguments of an inline if are compatible.
     * The 'then' and 'else' arguments are compatible if and only if they
     * have the same base type. In case of arrays, they must have the
     * same size and the subtypes must be compatible. In case of records,
     * they must have the same type name. */
    bool is_inline_if_compatible(const Type& alt1, const Type& alt2) const;

    /** Returns a value indicating the capabilities of a channel type. For
     * urgent channels this is 0, for non-urgent broadcast channels this
     * is 1, and in all other cases 2. */
    int channel_capability() const;

    /// Returns true if this and other are name-equivalent scalar types.
    bool is_same_scalar(const Type& other) const;

    /** Returns true iff this and other are structurally equivalent.
     * However, CONST, SYSTEM_META, and REF are ignored.
     * Scalar sets are checked using named equivalence.  */
    bool is_equivalent(const Type& other) const;

    /// Returns true if this is null-type or of kind UNKNOWN.
    bool unknown() const;

    /**
     * Replaces any LABEL labeled \a from occurring in the type
     * with a LABEL \a to. As always, a type is immutable, so a
     * copy of the type will be created.
     */
    Type rename(const std::string& from, const std::string& to) const;

    /**
     * Substitutes any occurence of \a symbol in any expression in
     * the type (expressions that occur as ranges either on
     * array sizes, scalars or integers) with \a expr.
     */
    Type subst(const Symbol& symbol, const Expression& expr) const;
    /**
     * Creates a new type by adding a prefix to it. The prefix
     * could be anything and it is the responsibility of the
     * caller to make sure that the given kind is a valid prefix.
     */
    Type create_prefix(Constants::Kind kind, position_t = position_t()) const;

    /// Creates a LABEL.
    Type create_label(std::string_view, position_t = position_t()) const;

    /// Creates a range based on primitive
    static Type create_range(Type primitive, Expression from, Expression till, position_t = position_t());

    /// Creates a primitive type
    static Type create_primitive(Constants::Kind, position_t = position_t());

    /// Creates an array type.
    static Type create_array(Type sub, Type size, position_t = position_t());

    /// Creates a new type definition.
    static Type create_typedef(std::string, Type, position_t = position_t());

    /// Creates a new process type.
    static Type create_process(const Frame&, position_t = position_t());

    /// Creates a new process-set type.
    static Type create_process_set(const Type& instance, position_t = position_t());

    /// Creates a new record type
    static Type create_record(const std::vector<Type>&, const std::vector<std::string>&,
                              const std::vector<position_t>&, position_t = position_t());

    /// Creates a new function type
    static Type create_function(Type, const std::vector<Type>&, const std::vector<std::string>&,
                                position_t = position_t());

    static Type create_external_function(Type rt, const std::vector<Type>&, const std::vector<std::string>&,
                                         position_t = position_t());

    /// Creates a new instance type
    static Type create_instance(const Frame&, position_t = position_t());
    /// Creates a new LSC instance type
    static Type create_LSC_instance(const Frame&, position_t = position_t());
    friend std::ostream& operator<<(std::ostream&, const Type&);
};
}  // namespace UTAP
#endif  // UTAP_TYPE_HH
