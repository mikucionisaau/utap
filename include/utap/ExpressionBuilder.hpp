// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2003 Uppsala University and Aalborg University.

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

#ifndef UTAP_EXPRESSIONBUILDER_HH
#define UTAP_EXPRESSIONBUILDER_HH

#include "AbstractBuilder.hpp"
#include "utap.hpp"

#include <stack>
#include <vector>
#include <cassert>

namespace UTAP {

constexpr auto defaultIntMin = -0x8000;
constexpr auto defaultIntMax = 0x7FFF;

template <typename T>
class FragmentStack
{
    std::vector<T> data;

public:
    T& operator[](int idx) { return data[data.size() - idx - 1]; }
    const T& operator[](int idx) const { return data[data.size() - idx - 1]; }
    T& top()
    {
        assert(!data.empty());
        return data.back();
    }
    const T& top() const
    {
        assert(!data.empty());
        return data.back();
    }
    void push(T e) { data.push_back(std::move(e)); }
    T pop()
    {
        assert(!data.empty());
        auto res = std::move(data.back());
        data.pop_back();
        return res;
    }
    void pop(size_t n)
    {
        assert(n <= size());
        while (n-- > 0)
            data.pop_back();
    }
    size_t size() const { return data.size(); }
    void duplicate()
    {
        assert(!data.empty());
        data.push_back(data.back());
    }
};

template <typename T>
T pop_top(std::stack<T>& stack)
{
    auto res = std::move(stack.top());
    stack.pop();
    return res;
}

template <typename T>
T pop_back(std::vector<T>& vec)
{
    auto res = std::move(vec.back());
    vec.pop_back();
    return res;
}

/**
 * Partial implementation of the builder interface: The
 * ExpressionBuilder implements all expression related
 * methods.
 *
 * In order to support quantifier expressions, this class also
 * handles the type related methods.
 *
 * This class does not implement any declaration related methods.
 *
 * Internally, three stacks are maintained: One for expressions,
 * one for types and for frames (scopes).
 */
class ExpressionBuilder : public AbstractBuilder
{
public:
    using ExpressionFragments = FragmentStack<Expression>;
    using TypeFragments = FragmentStack<Type>;

protected:
    /** Expression stack. */
    ExpressionFragments fragments;

    /** Type stack. */
    TypeFragments typeFragments;

    /** Frame stack. */
    std::stack<Frame> frames;

    /** Pointer to the document under construction. */
    Document& document;

    /** The template currently being parsed. */
    Template* currentTemplate{nullptr};

    /** Counter for creating unique scalarset names. */
    int32_t scalar_count = 0;

    /** Push a new frame. */
    void push_frame(Frame frame) { frames.push(std::move(frame)); }

    /** Pop the topmost frame. */
    Frame pop_frame()
    {
        auto res = std::move(frames.top());
        frames.pop();
        return res;
    }

    bool resolve(std::string_view, Symbol&) const;

    Expression make_constant(int value) const;
    Expression make_constant(double value) const;
    Expression make_constant(std::string_view value) const;

    /**
     * Given a prefix and a type, this method creates a new type
     * by applying the prefix. TypeExceptions might be thrown if
     * the combination of the prefix and the type is illegal.
     */
    Type apply_prefix(TypePrefix, Type type);

    /**
     * If this method returns true, it is allowed to access the
     * private identifiers of a process by prefixing the
     * identifier with the process name.
     *
     * This is only interesting when parsing properties. In this
     * case the method should be overridden by a sub class.
     */
    virtual bool allow_process_references() { return false; }
    std::map<std::string, Frame, std::less<>> dynamicFrames;

public:
    explicit ExpressionBuilder(Document& doc);
    ExpressionFragments& get_expressions() { return fragments; }

    void add_position(uint32_t position, uint32_t offset, uint32_t line, std::shared_ptr<std::string> path) override;

    void handle_error(const TypeException&) override;
    void handle_warning(const TypeException&) override;
    void type_duplicate() override;
    void type_pop() override;
    void type_bool(TypePrefix) override;
    void type_int(TypePrefix) override;
    void type_string(TypePrefix prefix) override;
    void type_double(TypePrefix) override;
    void type_bounded_int(TypePrefix) override;
    void type_channel(TypePrefix) override;
    void type_clock(TypePrefix) override;
    void type_void() override;
    void type_scalar(TypePrefix) override;
    void type_name(TypePrefix, std::string_view name) override;
    bool is_type(std::string_view) override;
    void expr_true() override;
    void expr_false() override;
    void expr_double(double) override;
    void expr_string(std::string_view name) override;
    void expr_identifier(std::string_view varName) override;
    void expr_location() override;
    void expr_nat(int32_t) override;
    void expr_call_begin() override;
    void expr_call_end(uint32_t n) override;
    void expr_array() override;
    void expr_post_increment() override;
    void expr_pre_increment() override;
    void expr_post_decrement() override;
    void expr_pre_decrement() override;
    void expr_assignment(Constants::Kind op) override;
    void expr_unary(Constants::Kind unaryop) override;
    void expr_binary(Constants::Kind binaryop) override;
    void expr_nary(Constants::Kind op, uint32_t num) override;
    void expr_scenario(std::string_view name) override;
    Expression exprScenario();
    void expr_ternary(Constants::Kind ternaryop, bool firstMissing) override;
    void expr_inline_if() override;
    void expr_comma() override;
    void expr_dot(std::string_view) override;
    void expr_deadlock() override;
    void expr_forall_begin(std::string_view name) override;
    void expr_forall_end(std::string_view name) override;
    void expr_exists_begin(std::string_view name) override;
    void expr_exists_end(std::string_view name) override;
    void expr_sum_begin(std::string_view name) override;
    void expr_sum_end(std::string_view name) override;

    void expr_proba_qualitative(Constants::Kind, Constants::Kind, double) override;
    void expr_proba_quantitative(Constants::Kind) override;
    void expr_proba_compare(Constants::Kind, Constants::Kind) override;
    void expr_proba_expected(std::string_view) override;
    void expr_builtin_function1(Constants::Kind) override;
    void expr_builtin_function2(Constants::Kind) override;
    void expr_builtin_function3(Constants::Kind) override;
    void expr_optimize_exp(Constants::Kind, PRICETYPE, Constants::Kind) override;
    void expr_save_strategy(std::string_view strategy_name) override;
    void expr_load_strategy() override;

    void expr_simulate(int nb_of_exprs, bool filter_prop = false, int max_accepting_runs = 0) override;
    void expr_MITL_formula() override;
    void expr_MITL_until(int, int) override;
    void expr_MITL_release(int, int) override;
    void expr_MITL_disj() override;
    void expr_MITL_conj() override;
    void expr_MITL_next() override;
    void expr_MITL_atom() override;
    void expr_MITL_diamond(int, int) override;
    void expr_MITL_box(int, int) override;

    /* Dynamic process creation */
    void expr_spawn(int params) override;
    void expr_exit() override;
    void expr_numof() override;
    void expr_forall_dynamic_begin(std::string_view, std::string_view) override;
    void expr_forall_dynamic_end(std::string_view name) override;
    void expr_exists_dynamic_begin(std::string_view, std::string_view) override;
    void expr_exists_dynamic_end(std::string_view) override;
    void expr_sum_dynamic_begin(std::string_view, std::string_view) override;
    void expr_sum_dynamic_end(std::string_view name) override;
    void expr_foreach_dynamic_begin(std::string_view, std::string_view) override;
    void expr_foreach_dynamic_end(std::string_view name) override;
    void push_dynamic_frame_of(Template* t, std::string_view name);  // no override
    void pop_dynamic_frame_of(std::string_view name);

    void builtin_decl_end() override;
};
}  // namespace UTAP

#endif
