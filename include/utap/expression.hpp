// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2011-2024 Aalborg University.
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

#ifndef UTAP_EXPRESSION_HH
#define UTAP_EXPRESSION_HH

#include "utap/common.hpp"
#include "utap/position.hpp"
#include "utap/string_interning.hpp"
#include "utap/symbols.hpp"

#include <memory>  // shared_ptr
#include <set>
#include <string_view>
#include <vector>

namespace UTAP {
/**
    A reference to an expression.

    An expression is a tree of operations and is identified by the
    root of the tree. There are many kinds of operations (see Kind),
    some are leafs in the tree (identifers, constants), some are
    unary operations (unary minus, negation), some are binary (plus,
    minus, etc.), and some are n-ary (lists, function calls).

    Expressions can be annotated with types. The type of an
    expression should never contain information about whether the
    expression is a variable reference, a constant value or about
    the range of the result value. Use the TypeChecker and
    RangeChecker classes to determine these informations.

    All nodes have the following attributes:

    kind     indicates the kind of operation
    position indicates the position of the expression in the input file.
    type     the type of the result of evaluating the expression
    size     the number of subexpressions
    sub      the subexpressions

    Some nodes have extra attributes (depending on the kind):

    value    the value of a CONSTANT node
    sync     the synchronisation of a SYNC node
    index    the index of a DOT node
    symbol   the symbol of an IDENTIFIER node

    Expressions are created by using the static factory methods.
*/

class Expression
{
private:
    struct expression_data;
    std::shared_ptr<expression_data> data = nullptr;  // PIMPL pattern with cheap/shallow copying
    Expression(Constants::Kind, const position_t&);

public:
    /// Default constructor creates an empty expression.
    Expression() = default;

    bool uses_fp() const;
    bool uses_clock() const;
    bool uses_hybrid() const;
    bool is_dynamic() const;
    bool has_dynamic_sub() const;
    /// Make a shallow clone of the expression.
    Expression clone() const;

    /// Makes a deep clone of the expression.
    Expression clone_deeper() const;

    /** Makes a deep clone of the expression and replaces the symbol
     * "from" with the symbol "to". */
    Expression clone_deeper(const Symbol& from, Symbol& to) const;

    /** Makes a deep clone of the expression and replaces each symbol
     * with a symbol from the given frame(s), with the same name */
    Expression clone_deeper(const Frame& frame, const Frame& select = {}) const;

    /// Returns the kind of the expression.
    Constants::Kind get_kind() const;

    /// Returns the number of subexpression.
    uint32_t get_size() const;

    /// Returns the position of this expression.
    const position_t& get_position() const;

    /// Returns the type of the expression.
    const Type& get_type() const;

    /// Sets the type of the expression.
    void set_type(Type);

    /** Returns the value field of this expression. This
        call is not valid for all expressions. */
    int32_t get_value() const;
    std::string_view get_string_value() const;
    size_t get_string_index() const;

    /** Returns an index into the record-labels
        Assumes that .get_type().is_process() or
                     .get_type().is_record()*/
    int32_t get_record_label_index() const;

    /** Returns the index field of this expression. */
    int32_t get_index() const;

    /** Returns the value field of this expression. This
        call is not valid for all expressions. */
    double get_double_value() const;

    /// Returns true if this is an empty expression.
    bool empty() const;

    /// Returns the synchronisation type of SYNC operations.
    Constants::Synchronisation get_sync() const;

    /// Outputs a textual representation of the expression.
    std::ostream& print(std::ostream& os, bool old = false) const;

    /// Returns a string representation of the expression.
    std::string str(bool old = false) const;

    /// Returns the ith subexpression.
    Expression& operator[](uint32_t);

    /// Returns the ith subexpression.
    const Expression& operator[](uint32_t) const;

    /// Returns the ith subexpression.
    Expression& get(uint32_t);

    /// Returns the ith subexpression.
    const Expression& get(uint32_t) const;

    /// Equality operator
    bool equal(const Expression&) const;

    /** Returns the symbol of a variable reference. The expression
     *  must be a left-hand side value. In case of
     *  dot-expressions, the record/process symbol is returned. In
     *  case of an inline if, the 'true' branch is returned.
     *
     *  (a=1).get_symbol() returns 'a'
     *  (s.f).get_symbol() returns 's'
     *  (i<1?j:k).get_symbol() returns 'j'
     */
    // const Symbol& get_symbol();

    /** Returns the set of symbols this expression might resolve
     * into. In case of inline if, both the 'true' and 'false'
     * branch is added. In case of dot-expressions, both the left
     * hand reference and the member field are returned.
     *
     * (a=1).get_symbol() returns 'a'
     * (s.f).get_symbol() returns 's,f'
     * (i<1?j:k).get_symbol() returns 'j,k'
     */
    void get_symbols(std::set<Symbol>& symbols) const;

    /** Returns the symbol this expression evaluates to. Notice
        that not all expression evaluate to a symbol. */
    Symbol get_symbol() const;

    /** Returns true if this expression is a reference to a
        symbol in the given set. */
    bool is_reference_to(const std::set<Symbol>&) const;

    /// Returns true if the expression contains deadlock expression
    bool contains_deadlock() const;
    /** True if this expression can change any of the variables
            identified by the given symbols. */
    bool changes_variable(const std::set<Symbol>&) const;

    /// True if this expression can change any variable at all.
    bool changes_any_variable() const;

    /** True if the evaluation of this expression depends on
        any of the symbols in the given set. */
    bool depends_on(const std::set<Symbol>&) const;

    void collect_possible_writes(std::set<Symbol>&) const;
    void collect_possible_reads(std::set<Symbol>&, bool collectRandom = false) const;

    /** Less-than operator. Makes it possible to put Expression
        objects into an STL set. */
    bool operator<(const Expression& other) const
    {
        return data != nullptr && other.data != nullptr && data < other.data;
    }

    /** Equality operator. Returns true if the two references point
        to the same expression object. */
    bool operator==(const Expression& other) const { return data == other.data; }

    Expression subst(const Symbol&, Expression) const;

    /// Precedence of expression type, higher precedence goes before low precedence
    static int get_precedence(Constants::Kind);

    /// Create a CONSTANT expression.
    static Expression create_constant(int32_t, position_t = {});
    static Expression create_var_index(int32_t, position_t = {});

    static Expression create_double(double, position_t = {});
    /// Life time of string reference must outlive expression
    static Expression create_string(StringIndex, position_t = {});

    /// Create an IDENTIFIER expression
    static Expression create_identifier(const Symbol&, position_t = {});

    /// Create a unary expression
    static Expression create_unary(Constants::Kind, Expression, position_t = {}, Type = {});

    /** Create a binary expression */
    static Expression create_binary(Constants::Kind, Expression, Expression, position_t = {}, Type = {});

    /** Create a ternary expression */
    static Expression create_ternary(Constants::Kind, Expression, Expression, Expression, position_t = {}, Type = {});

    /** Create an n-ary expression */
    static Expression create_nary(Constants::Kind, std::vector<Expression> sub, position_t = {}, Type = {});

    /** Create a DOT expression */
    static Expression create_dot(Expression, int32_t index, position_t = {}, Type = {});

    /** Create a SYNC expression */
    static Expression create_sync(Expression, Constants::Synchronisation, position_t = {});

    /** Create a DEADLOCK expression */
    static Expression create_deadlock(position_t = {});

    static Expression create_exit(position_t = {});

    // true if empty or equal to 1.
    bool is_true() const;
    int get_precedence() const;
    friend std::ostream& operator<<(std::ostream& o, const UTAP::Expression& e) { return e.print(o); }

private:
    std::ostream& print_bound_type(std::ostream& os, const Expression& e) const;
};

}  // namespace UTAP

#endif  // UTAP_EXPRESSION_HH
