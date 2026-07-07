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
#include "utap/TypeChecker.hpp"

#include "utap/DocumentBuilder.hpp"
#include "utap/ExpressionBuilder.hpp"
#include "utap/FeatureChecker.hpp"
#include "utap/builder.hpp"
#include "utap/common.hpp"
#include "utap/document.hpp"
#include "utap/expression.hpp"
#include "utap/symbols.hpp"
#include "utap/utap.hpp"

#include <algorithm>
#include <filesystem>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <cassert>
#include <cstddef>  // size_t
#include <cstdint>  // uint32_t
#include <cstdio>

using namespace UTAP;
using namespace Constants;

using namespace std::string_literals;

namespace {  // anonymous namespace is preferred over `static` (C++ standard)

/// The following are simple helper functions for testing the type of expressions.
bool is_cost(const Expression& expr) { return expr.get_type().is(COST); }

bool is_void(const Expression& expr) { return expr.get_type().is_void(); }

bool is_double(const Expression& expr) { return expr.get_type().is_double(); }

/*
bool is_string(Expression expr)
{
    return expr.get_type().is_string();
}
*/
// bool is_scalar(Expression expr)
// {
//     return expr.get_type().is_scalar();
// }

bool is_integer(const Expression& expr) { return expr.get_type().is_integer(); }

bool is_const_integer(const Expression& expr) { return expr.get_kind() == CONSTANT && is_integer(expr); }

bool is_default_int(const Type& type)
{
    if (type.is_integral()) {
        auto range = type.get_range();
        return (is_const_integer(range.first) && is_const_integer(range.second)) &&
               range.first.get_value() == defaultIntMin && range.second.get_value() == defaultIntMax;
    }
    return false;
}

bool is_bound(const Expression& expr) { return expr.get_type().is_integer() || expr.get_type().is_double(); }

bool is_integral(const Expression& expr) { return expr.get_type().is_integral(); }

bool is_clock(const Expression& expr) { return expr.get_type().is_clock(); }

bool is_diff(const Expression& expr) { return expr.get_type().is_diff(); }

bool is_double_value(const Expression& expr) { return is_double(expr) || is_clock(expr) || is_diff(expr); }

bool is_number(const Expression& expr) { return is_double_value(expr) || is_integral(expr); }

bool is_const_double(const Expression& expr) { return expr.get_kind() == CONSTANT && is_double(expr); }

bool is_invariant(const Expression& expr) { return expr.get_type().is_invariant(); }

bool is_guard(const Expression& expr) { return expr.get_type().is_guard(); }

bool is_probability(const Expression& expr) { return expr.get_type().is_probability(); }

bool is_constraint(const Expression& expr) { return expr.get_type().is_constraint(); }

bool is_formula(const Expression& expr) { return expr.get_type().is_formula(); }

bool is_formula_list(const Expression& expr)
{
    if (expr.get_kind() != LIST) {
        return false;
    }

    for (uint32_t i = 0; i < expr.get_size(); ++i) {
        if (!expr[i].get_type().is_formula()) {
            return false;
        }
    }

    return true;
}

bool hasStrictLowerBound(const Expression& expr)
{
    for (uint32_t i = 0; i < expr.get_size(); ++i) {
        if (hasStrictLowerBound(expr[i])) {
            return true;
        }
    }

    switch (expr.get_kind()) {
    case LT:  // int < clock
        if (is_integral(expr[0]) && is_clock(expr[1])) {
            return true;
        }
        break;
    case GT:  // clock > int
        if (is_clock(expr[0]) && is_integral(expr[1])) {
            return true;
        }
        break;

    default:;
    }

    return false;
}

bool hasStrictUpperBound(const Expression& expr)
{
    for (uint32_t i = 0; i < expr.get_size(); ++i) {
        if (hasStrictUpperBound(expr[i])) {
            return true;
        }
    }

    switch (expr.get_kind()) {
    case GT:  // int > clock
        if (is_integral(expr[0]) && is_clock(expr[1])) {
            return true;
        }
        break;
    case LT:  // clock < int
        if (is_clock(expr[0]) && is_integral(expr[1])) {
            return true;
        }
        break;

    default:;
    }

    return false;
}

/**
 * Returns true iff type is a valid invariant. A valid invariant is
 * either an invariant expression or an integer expression.
 */
bool is_invariant_wr(const Expression& expr) { return is_invariant(expr) || (expr.get_type().is(INVARIANT_WR)); }

/**
 * Returns true if values of this type can be assigned. This is the
 * case for integers, booleans, clocks, cost, scalars and arrays and
 * records of these. E.g. channels and processes are not assignable.
 */
bool is_assignable(const Type& type)
{
    switch (type.get_kind()) {
    case Constants::INT:
    case Constants::BOOL:
    case Constants::DOUBLE:
    case Constants::STRING:
    case Constants::CLOCK:
    case Constants::COST:
    case Constants::SCALAR: return true;

    case ARRAY: return is_assignable(type[0]);

    case RECORD:
        for (uint32_t i = 0; i < type.size(); i++) {
            if (!is_assignable(type[i])) {
                return false;
            }
        }
        return true;

    default: return type.size() > 0 && is_assignable(type[0]);
    }
}

using Error = TypeChecker::TypeError;

Error expression_has_no_effect(const Expression& expr) { return {expr, "$Expression_does_not_have_any_effect"}; }

Error urgent_only_for_locations_and_channels(const Type& type)
{
    return {type, "$Prefix_urgent_only_allowed_for_locations_and_channels"};
}

Error broadcast_only_for_channels(const Type& type) { return {type, "$Prefix_broadcast_only_allowed_for_channels"}; }

Error committed_only_for_locations(const Type& type) { return {type, "$Prefix_committed_only_allowed_for_locations"}; }

Error hybrid_only_for_clocks(const Type& type) { return {type, "$Prefix_hybrid_only_allowed_for_clocks"}; }

Error const_not_allowed_for_clocks(const Type& type) { return {type, "$Prefix_const_not_allowed_for_clocks"}; }

Error meta_not_allowed_for_clocks(const Type& type) { return {type, "$Prefix_meta_not_allowed_for_clocks"}; }

Error reference_is_not_allowed(const Type& type) { return {type, "$Reference_to_this_type_is_not_allowed"}; }

Error range_is_not_allowed(const Type& type) { return {type, "$Range_over_this_type_is_not_allowed"}; }

Error must_be_computable_at_compile_time(const Expression& expr)
{
    return {expr, "$Must_be_computable_at_compile_time"};
}

Error must_be_false(const Expression& expr) { return {expr, "$Must_be_false"}; }

Error invalid_array_size(const Type& type) { return {type, "$Invalid_array_size"}; }

Error invalid_return_type(const Type& type) { return {type, "$Invalid_return_type"}; }

Error cannot_be_inside_struct(const Type& type) { return {type, "$Type_cannot_be_declared_inside_a_struct"}; }

Error cannot_be_const_or_meta(const Type& type) { return {type, "$Type_cannot_be_declared_const_or_meta"}; }

Error must_be_side_effect_free(const Expression& expr) { return {expr, "$Expression_must_be_side-effect_free"}; }

Error must_be_valid_formula(const Expression& expr) { return {expr, "$Property_must_be_a_valid_formula"}; }

Error progress_guard_must_evaluate_to_boolean(const Expression& expr)
{
    return {expr, "$Progress_guard_must_evaluate_to_a_boolean_value"};
}

Error progress_measure_must_evaluate_to_integer(const Expression& expr)
{
    return {expr, "$Progress_measure_must_evaluate_to_a_integer_value"};
}

Error boolean_expected(const Expression& expr) { return {expr, "$Boolean_expected"}; }

Error integer_expected(const Expression& expr) { return {expr, "$Integer_expected"}; }

Error integer_or_clock_expected(const Expression& expr) { return {expr, "$Integer_or_clock_expected"}; }

Error array_expected(const Expression& expr) { return {expr, "$Array_expected"}; }

Error floating_point_expected(const Expression& expr) { return {expr, "$Floating_point_expected"}; }

Error loadStrategy_and_saveStrategy_expect_string(const Expression& expr)
{
    return {expr, "$loadStrategy_and_saveStrategy_expect_a_string"};
}

Error incompatible_argument(const Expression& expr) { return {expr, "$Incompatible_argument"}; }

Error incompatible_arguments_to_inline_if(const Expression& expr)
{
    return {expr, "$Incompatible_arguments_to_inline_if"};
}

Error incompatible_type_for_comma(const Expression& expr) { return {expr, "$Incompatible_type_for_comma_expression"}; }

Error channel_expected(const Expression& expr) { return {expr, "$Channel_expected"}; }

Error clock_expected(const Expression& expr) { return {expr, "$Clock_expected"}; }

Error scalar_or_integer_expected(const Type& type) { return {type, "$Scalar_set_or_integer_expected"}; }

Error range_expected(const Type& type) { return {type, "$Range_expected"}; }

Error csp_and_io_cannot_be_mixed(const Expression& expr)
{
    return {expr, "$CSP_and_IO_synchronisations_cannot_be_mixed"};
}

Error csp_sync_is_incompatible_with_refinement_checking(const Expression& expr)
{
    return {expr, "$CSP_synchronisations_are_incompatible_with_refinement_checking"};
}

Error free_param_must_be_int_or_scalar(const Symbol& symbol)
{
    return {symbol, "$Free_process_parameters_must_be_a_bounded_integer_or_a_scalar", symbol.get_type().str()};
}

Error free_param_must_not_be_used_in_array_or_select(const Symbol& symbol)
{
    return {symbol,
            "$Free_process_parameters_must_not_be_used_directly_or_indirectly_in_"
            "an_array_declaration_or_select_expression",
            symbol.get_name()};
}

Error dynamic_constructions_cannot_initialize(const Expression& expr)
{
    return {expr, "Dynamic constructions cannot be used as initialisers"};
}

Error cannot_be_used_as_invariant(const Expression& expr)
{
    return {expr, "$Expression_of_type "s + expr.get_type().str() + " $cannot_be_used_as_an_invariant"};
}

Error cannot_be_used_as_guard(const Expression& expr)
{
    return {expr, "$Expression_of_type "s + expr.get_type().str() + " $cannot_be_used_as_a_guard"};
}

Error cannot_be_used_as_probability(const Expression& expr)
{
    return {expr, "$Expression_of_type " + expr.get_type().str() + " $cannot_be_used_as_a_probability"};
}

Error cannot_be_used_as_condition(const Expression& expr)
{
    return {expr, "$Expression_of_type " + expr.get_type().str() + " $cannot_be_used_as_a_condition"};
}

Error field_name_not_allowed_in_array_init(const Expression& expr)
{
    return {expr, "$Field_name_not_allowed_in_array_initialiser"};
}

Error unknown_field_name(const Expression& expr) { return {expr, "$Unknown_field_name"}; }

Error too_many_elements_in_initializer(const Expression& expr) { return {expr, "$Too_many_elements_in_initialiser"}; }

Error multiple_initializers_for_field(const Expression& expr) { return {expr, "$Multiple_initialisers_for_field"}; }

Error incomplete_initializer(const Expression& expr) { return {expr, "$Incomplete_initialiser"}; }

Error invalid_initializer(const Expression& expr) { return {expr, "$Invalid_initialiser"}; }

Error invalid_sum(const Expression& expr)
{
    return {expr, "$Sum_can_only_be_over_integer,_double,_invariant_or_guard_expressions"};
}

Error cannot_spawn_a_non_dynamic_template(const Expression& expr)
{
    return {expr, "$Cannot_spawn_a_non-dynamic_template"};
}

Error wrong_number_of_arguments(const Expression& expr) { return {expr, "$Wrong_number_of_arguments"}; }

Error bug_wrong_number_of_args(const Expression& expr) { return {expr, "$Bug:_wrong_number_of_arguments"}; }

Error bug_bad_path_quantifier(const Expression& expr) { return {expr, "$Bug:_bad_path_quantifier"}; }

Error bug_bad_aggregation_expression(const Expression& expr)
{
    return {expr, "$Bug:_bad_aggregation_operator_expression"};
}

Error bug_bad_aggregation_value(const Expression& expr) { return {expr, "$Bug:_bad_aggregation_operator_value"}; }

Error unknown_type(const Expression& expr) { return {expr, "$Unknown_type_of_the_expression"}; }

Error explicit_number_of_runs_is_not_supported_here(const Expression& expr)
{
    return {expr, "$Explicit_number_of_runs_is_not_supported_here"};
}

Error template_only_declared_and_undefined(const Expression& expr)
{
    return {expr, "$Template_is_only_declared_and_not_defined"};
}

Error not_dynamic_template(const Expression& expr) { return {expr, "$Not_a_dynamic_template"}; }

Error exit_only_in_dynamic_template(const Expression& expr)
{
    return {expr, "$Exit_can_only_be_used_in_dynamic_templates"};
}

Error incompatible_type(const Expression& expr) { return {expr, "$Incompatible_type"}; }

Error incompatible_types(const Expression& expr) { return {expr, "$Incompatible_types"}; }

Error dynamic_constructs_supported_only_on_edges(const Expression& expr)
{
    return {expr, "$Dynamic_constructs_supported_only_on_edges"};
}

Error lvalue_expected(const Expression& expr) { return {expr, "$Left_hand_side_value_expected"}; }

Error use_regular_assignment_on_non_integer(const Expression& expr)
{
    return {expr, "$Non-integer_types_must_use_regular_assignment_operator"};
}

Error increment_only_integers_and_cost(const Expression& expr)
{
    return {expr, "$Increment_can_only_be_used_for_integers_and_cost_variables"};
}

Error only_one_cost_rate_is_allowed(const Expression& expr) { return {expr, "$Only_one_cost_rate_is_allowed"}; }

Error strict_invariant(const Expression& expr) { return {expr, "$Strict_invariant"}; }

Error number_expected(const Expression& expr) { return {expr, "$Number_expected"}; }

Error first_argument_of_inline_if_must_be_integer(const Expression& expr)
{
    return {expr, "$First_argument_of_inline_if_must_be_an_integer"};
}

Error deprecated_reset(const Symbol& symbol)
{
    return {symbol, "Deprecated __RESET__ annotation: use \"{ integers } -> { floats }\" in learning query.",
            symbol.get_name()};
}

Error clock_guards_not_allowed_on_urgent_edges(const Expression& expr)
{
    return {expr, "$Clock_guards_are_not_allowed_on_urgent_edges"};
}

Error input_edges_must_be_deterministic(const Expression& expr)
{
    return {expr, "SMC requires input edges to be deterministic"};
}

Error may_need_guard_involving_target_invariant(const Expression& expr)
{
    return {expr, "$It_may_be_needed_to_add_a_guard_involving_the_target_invariant"};
}

Error strict_bounds_on_urgent_edges(const Expression& expr)
{
    return {expr, "$Strict_bounds_on_urgent_edges_may_not_make_sense"};
}

Error outputs_should_be_uncontrollable(const Expression& expr)
{
    return {expr, "$Outputs_should_be_uncontrollable_for_refinement_checking"};
}

Error inputs_should_be_controllable(const Expression& expr)
{
    return {expr, "$Inputs_should_be_controllable_for_refinement_checking"};
}

Error nested_path_quntifiers_not_supported(const Expression& expr)
{
    return {expr, "$Nested_path_quantifiers_are_not_supported"};
}

Error mitl_inside_forall_or_exists_in_non_mitl(const Expression& expr)
{
    return {expr, "$Unsupported_MITL_inside_forall_or_exists_in_non_MITL_property"};
}

Error invalid_assignment(const Expression& expr) { return {expr, "$Invalid_assignment_expression"}; }

Error clock_lower_and_upper_bounds(const Expression& expr)
{
    return {expr, "$Clock_lower_bound_must_be_weak_and_upper_bound_strict"};
}

Error clock_difference_is_not_supported(const Expression& expr)
{
    return {expr, "$Clock_differences_are_not_supported"};
}

bool is_game_property(const Expression& expr)
{
    switch (expr.get_kind()) {
    case CONTROL:
    case SMC_CONTROL:
    case EF_CONTROL:
    case CONTROL_TOPT:
    case PO_CONTROL:
    case CONTROL_TOPT_DEF1:
    case CONTROL_TOPT_DEF2: return true;
    default: return false;
    }
}

bool has_MITL_in_quantified_sub(const Expression& expr)
{
    bool hasIt = (expr.get_kind() == MITL_FORALL || expr.get_kind() == MITL_EXISTS);
    if (!hasIt) {
        for (uint32_t i = 0; i < expr.get_size(); i++) {
            hasIt |= has_MITL_in_quantified_sub(expr.get(i));
        }
    }
    return hasIt;
}

bool has_spawn_or_exit(const Expression& expr)
{
    bool hasIt = (expr.get_kind() == SPAWN || expr.get_kind() == EXIT);
    if (!hasIt) {
        for (uint32_t i = 0; i < expr.get_size(); i++) {
            hasIt |= has_spawn_or_exit(expr.get(i));
        }
    }
    return hasIt;
}

bool valid_return_type(const Type& type)
{
    switch (type.get_kind()) {
    case Constants::RECORD:
        for (uint32_t i = 0; i < type.size(); i++) {
            if (!valid_return_type(type[i])) {
                return false;
            }
        }
        return true;

    case Constants::RANGE:
    case Constants::LABEL: return valid_return_type(type[0]);

    case Constants::INT:
    case Constants::BOOL:
    case Constants::SCALAR:
    case Constants::DOUBLE: return true;

    default: return false;
    }
}

void static_analysis(Document& doc)
{
    if (!doc.has_errors()) {
        auto checker = TypeChecker{doc};
        doc.accept(checker);
        auto fchecker = FeatureChecker{doc};
        doc.set_supported_methods(fchecker.get_supported_methods());
    }
}
}  // anonymous namespace

///////////////////////////////////////////////////////////////////////////

void CompileTimeComputableValues::visit_variable(Variable& variable)
{
    if (variable.uid.get_type().is_constant()) {
        variables.insert(variable.uid);
    }
}

void CompileTimeComputableValues::visit_instance(Instance& temp)
{
    for (const auto& param : temp.parameters) {
        const Type& type = param.get_type();
        if (!type.is(REF) && type.is_constant() && !type.is_double()) {
            variables.insert(param);
        }
    }
}

void CompileTimeComputableValues::add_symbol(Symbol symbol) { variables.insert(std::move(symbol)); }

bool CompileTimeComputableValues::contains(const Symbol& symbol) const
{
    return (variables.find(symbol) != variables.end());
}

///////////////////////////////////////////////////////////////////////////

class RateDecomposer
{
public:
    RateDecomposer() = default;
    Expression costRate;
    Expression invariant{Expression::create_constant(1)};
    bool hasStrictInvariant{false}, hasClockRates{false};
    size_t countCostRates{0};

    void decompose(const Expression& expr, bool inforall = false);
};

void RateDecomposer::decompose(const Expression& expr, bool inforall)
{
    assert(is_invariant_wr(expr));

    if (is_invariant(expr)) {
        if (expr.get_kind() == Constants::LT) {
            hasStrictInvariant = true;  // Strict upper bounds only.
        }
        if (!inforall) {
            invariant = invariant.empty()
                            ? expr
                            : invariant = Expression::create_binary(AND, invariant, expr, expr.get_position(),
                                                                    Type::create_primitive(INVARIANT));
        }
    } else if (expr.get_kind() == AND) {
        decompose(expr[0], inforall);
        decompose(expr[1], inforall);
    } else if (expr.get_kind() == EQ) {
        Expression left;
        Expression right;
        assert((expr[0].get_type().get_kind() == RATE) ^ (expr[1].get_type().get_kind() == RATE));

        if (expr[0].get_type().get_kind() == RATE) {
            left = expr[0][0];
            right = expr[1];
        } else {
            left = expr[1][0];
            right = expr[0];
        }

        if (is_cost(left)) {
            costRate = right;
            countCostRates++;
        } else {
            hasClockRates = true;
            if (!inforall) {
                invariant = invariant.empty() ? expr
                                              : Expression::create_binary(AND, invariant, expr, expr.get_position(),
                                                                          Type::create_primitive(INVARIANT_WR));
            }
        }
    } else {
        assert(expr.get_type().is(INVARIANT_WR));
        assert(expr.get_kind() == FORALL);
        // Enter the forall to look for clock rates but don't
        // record them, rather the forall expression.
        decompose(expr[1], true);
        invariant = invariant.empty() ? expr
                                      : invariant = Expression::create_binary(AND, invariant, expr, expr.get_position(),
                                                                              Type::create_primitive(INVARIANT_WR));
    }
}

///////////////////////////////////////////////////////////////////////////

TypeChecker::TypeChecker(Document& document, bool refinement): document{document}, refinementWarnings{refinement}
{
    document.accept(compileTimeComputableValues);

    checkExpression(document.get_before_update());
    checkExpression(document.get_after_update());
}

void TypeChecker::handleWarning(TypeError warning)
{
    document.add_warning(warning.position, std::move(warning.message), std::move(warning.context));
}

void TypeChecker::handleError(TypeError error)
{
    document.add_error(error.position, std::move(error.message), std::move(error.context));
}

/**
 * This method issues warnings for expressions, which do not change
 * any variables. It is expected to be called for all expressions
 * whose value is ignored. Unless such an expression has some kind
 * of side-effect, it does not have any purpose.
 *
 * Notice that in contrast to the regular side-effect analysis,
 * this function accepts modifications of local variables as a
 * side-effect.
 */
void TypeChecker::checkIgnoredValue(const Expression& expr)
{
    if (!expr.changes_any_variable() && expr.get_kind() != FUN_CALL_EXT) {
        handleWarning(expression_has_no_effect(expr));
    } else if (expr.get_kind() == COMMA && !expr[1].changes_any_variable() && expr[1].get_kind() != FUN_CALL_EXT) {
        handleWarning(expression_has_no_effect(expr[1]));
    }
}

bool TypeChecker::isCompileTimeComputable(const Expression& expr) const
{
    /* An expression is compile time computable if all identifers it
     * could possibly access during an evaluation are compile time
     * computable (i.e. their value is known at compile time).
     *
     * FIXME: We could maybe refine this to actually include function
     * local variables with compile time computable initialisers. This
     * would increase the class of models we accept while also getting
     * rid of the compileTimeComputableValues object.
     */
    auto reads = std::set<Symbol>{};
    expr.collect_possible_reads(reads, true);
    return std::all_of(reads.begin(), reads.end(), [this](const Symbol& s) {
        return s != Symbol{} && (s.get_type().is_function() || s.get_type().is_function_external() ||
                                 compileTimeComputableValues.contains(s));
    });
}

/**
 * Check that the type is valid, i.e.:
 *
 * - all expressions such as array sizes, integer ranges, etc. are
 *   type correct,
 *
 * - only allowed prefixes are used (e.g. no urgent integers).
 *
 * - array sizes and integer bounds are compile time computable.
 *
 * If \a initialisable is true, then this method also checks that \a
 * type is initialisable.
 */
void TypeChecker::checkType(const Type& type, bool initialisable, bool inStruct)
{
    Expression l;
    Expression u;
    Type size;

    switch (type.get_kind()) {
    case LABEL: checkType(type[0], initialisable, inStruct); break;

    case URGENT:
        if (!type.is_location() && !type.is_channel())
            handleError(urgent_only_for_locations_and_channels(type));
        checkType(type[0], initialisable, inStruct);
        break;

    case BROADCAST:
        if (!type.is_channel())
            handleError(broadcast_only_for_channels(type));
        checkType(type[0], initialisable, inStruct);
        break;

    case COMMITTED:
        if (!type.is_location())
            handleError(committed_only_for_locations(type));
        checkType(type[0], initialisable, inStruct);
        break;

    case HYBRID:
        if (!type.is_clock() && !(type.is_array() && type.strip_array().is_clock()))
            handleError(hybrid_only_for_clocks(type));
        checkType(type[0], initialisable, inStruct);
        break;

    case CONSTANT:
        if (type.is_clock())
            handleError(const_not_allowed_for_clocks(type));
        checkType(type[0], true, inStruct);
        break;

    case SYSTEM_META:
        if (type.is_clock())
            handleError(meta_not_allowed_for_clocks(type));
        checkType(type[0], true, inStruct);
        break;

    case REF:
        if (!type.is_integral() && !type.is_array() && !type.is_record() && !type.is_channel() && !type.is_clock() &&
            !type.is_scalar() && !type.is_double() && !type.is_string())
            handleError(reference_is_not_allowed(type));
        checkType(type[0], initialisable, inStruct);
        break;

    case RANGE:
        if (!type.is_integer() && !type.is_scalar()) {
            handleError(range_is_not_allowed(type));
        }
        std::tie(l, u) = type.get_range();
        if (checkExpression(l)) {
            if (!is_integer(l))
                handleError(integer_expected(l));
            if (!isCompileTimeComputable(l))
                handleError(must_be_computable_at_compile_time(l));
        }
        if (checkExpression(u)) {
            if (!is_integer(u))
                handleError(integer_expected(u));
            if (!isCompileTimeComputable(u))
                handleError(must_be_computable_at_compile_time(u));
        }
        break;

    case ARRAY:
        size = type.get_array_size();
        if (!size.is(RANGE))
            handleError(invalid_array_size(type));
        else
            checkType(size);
        checkType(type[0], initialisable, inStruct);
        break;

    case RECORD:
        for (uint32_t i = 0; i < type.size(); i++) {
            checkType(type.get_sub(i), true, true);
        }
        break;
    case Constants::STRING:
        if (inStruct)
            handleError(cannot_be_inside_struct(type));
		break;
    case Constants::CLOCK: break;
    case Constants::DOUBLE: break;
    case Constants::INT: break;
    case Constants::BOOL: break;

    default:
        if (initialisable)
            handleError(cannot_be_const_or_meta(type));
    }
}

void TypeChecker::visit_doc_after(Document& doc)
{
    for (ChanPriority& i : doc.get_chan_priorities()) {
        const bool i_default = (i.head == Expression());
        if (!i_default && checkExpression(i.head)) {
            Expression expr = i.head;
            Type channel = expr.get_type();

            // Check that chanElement is a channel, or an array of channels.
            while (channel.is_array()) {
                channel = channel.get_sub();
            }
            if (!channel.is_channel()) {
                handleError(channel_expected(expr));
            }

            // Check index expressions
            while (expr.get_kind() == ARRAY) {
                if (!isCompileTimeComputable(expr[1]))
                    handleError(must_be_computable_at_compile_time(expr[1]));
                else if (i.head.changes_any_variable())
                    handleError(must_be_side_effect_free(expr[1]));
                expr = expr[0];
            }
        }

        for (ChanPriority::Entry& j : i.tail) {
            const bool j_default = (j.second == Expression());
            if (!j_default && checkExpression(j.second)) {
                Expression expr = j.second;
                Type channel = expr.get_type();

                // Check that chanElement is a channel, or an array of channels.
                while (channel.is_array())
                    channel = channel.get_sub();
                if (!channel.is_channel())
                    handleError(channel_expected(expr));

                // Check index expressions
                while (expr.get_kind() == ARRAY) {
                    if (!isCompileTimeComputable(expr[1]))
                        handleError(must_be_computable_at_compile_time(expr[1]));
                    else if (j.second.changes_any_variable())
                        handleError(must_be_side_effect_free(expr[1]));
                    expr = expr[0];
                }
            }
        }
    }
}

void TypeChecker::visitHybridClock(Expression& e)
{
    if (checkExpression(e)) {
        if (!is_clock(e)) {
            handleError(clock_expected(e));
        } else if (e.changes_any_variable())
            handleError(must_be_side_effect_free(e));
        // Should be a check to identify the clock at compile time.
        // Problematic now. Same issue for inf & sup.
    }
}

void TypeChecker::visit_io_decl(IODecl& iodecl)
{
    for (auto& e : iodecl.param) {
        if (checkExpression(e)) {
            if (!is_integer(e))
                handleError(integer_expected(e));
            else if (!isCompileTimeComputable(e))
                handleError(must_be_computable_at_compile_time(e));
            else if (e.changes_any_variable())
                handleError(must_be_side_effect_free(e));
        }
    }

    if (syncUsed == 0) {
        if (!iodecl.inputs.empty() || !iodecl.outputs.empty()) {
            syncUsed = 1;
        } else if (!iodecl.csp.empty()) {
            syncUsed = 2;
        }
    }
    if (syncUsed == 1) {
        if (!iodecl.csp.empty()) {
            syncUsed = -1;
        }
    } else if (syncUsed == 2) {
        if (!iodecl.inputs.empty() || !iodecl.outputs.empty()) {
            syncUsed = -1;
        }
    }
    if (syncUsed == -1)
        handleError(csp_and_io_cannot_be_mixed(iodecl.csp.front()));

    document.set_sync_used(syncUsed);

    for (auto expr : iodecl.inputs) {  // intentional copy
        if (checkExpression(expr)) {
            Type channel = expr.get_type();

            // Check that chanElement is a channel, or an array of channels.
            while (channel.is_array())
                channel = channel.get_sub();
            if (!channel.is_channel())
                handleError(channel_expected(expr));

            // Check index expressions
            while (expr.get_kind() == ARRAY) {
                if (!isCompileTimeComputable(expr[1]))
                    handleError(must_be_computable_at_compile_time(expr[1]));
                else if (expr.changes_any_variable())
                    handleError(must_be_side_effect_free(expr[1]));
                expr = expr[0];
            }
        }
    }

    for (auto expr : iodecl.outputs) {  // intentional copy
        if (checkExpression(expr)) {
            Type channel = expr.get_type();

            // Check that chanElement is a channel, or an array of channels.
            while (channel.is_array())
                channel = channel.get_sub();
            if (!channel.is_channel())
                handleError(channel_expected(expr));

            // Check index expressions
            while (expr.get_kind() == ARRAY) {
                if (!isCompileTimeComputable(expr[1]))
                    handleError(must_be_computable_at_compile_time(expr[1]));
                else if (expr.changes_any_variable())
                    handleError(must_be_side_effect_free(expr[1]));
                expr = expr[0];
            }
        }
    }
}

void TypeChecker::visit_process(Instance& process)
{
    for (uint32_t i = 0; i < process.unbound; i++) {
        // Unbound parameters of processes must be either scalars or bounded integers.
        const Symbol& parameter = process.parameters[i];
        const Type& type = parameter.get_type();
        if (!(type.is_scalar() || type.is_range()) || type.is(REF) || is_default_int(type))
            handleError(free_param_must_be_int_or_scalar(parameter));
        /* Unbound parameters must not be used either directly or indirectly in any array size declarations.
         * I.e. they must not be restricted. */
        if (process.restricted.find(parameter) != process.restricted.end())
            handleError(free_param_must_not_be_used_in_array_or_select(parameter));
    }
}

void TypeChecker::visit_variable(Variable& variable)
{
    DocumentVisitor::visit_variable(variable);

    checkType(variable.uid.get_type());
    if (variable.init.is_dynamic() || variable.init.has_dynamic_sub()) {
        handleError(dynamic_constructions_cannot_initialize(variable.init));
    } else if (!variable.init.empty() && checkExpression(variable.init)) {
        if (!isCompileTimeComputable(variable.init))
            handleError(must_be_computable_at_compile_time(variable.init));
        else if (variable.init.changes_any_variable())
            handleError(must_be_side_effect_free(variable.init));
        else
            checkInitialiser(variable.uid.get_type(), variable.init);
    }
}

void TypeChecker::visit_location(Location& loc)
{
    DocumentVisitor::visit_location(loc);

    if (!loc.invariant.empty()) {
        auto& inv = loc.invariant;
        if (checkExpression(inv)) {
            if (!is_invariant_wr(inv))
                handleError(cannot_be_used_as_invariant(inv));
            else if (inv.changes_any_variable())
                handleError(must_be_side_effect_free(inv));
            else {
                auto decomposer = RateDecomposer{};
                decomposer.decompose(inv);
                inv = decomposer.invariant;
                loc.cost_rate = decomposer.costRate;
                if (decomposer.countCostRates > 1) {
                    handleError(only_one_cost_rate_is_allowed(inv));
                }
                if (decomposer.hasClockRates) {
                    document.record_stop_watch();
                }
                if (decomposer.hasStrictInvariant) {
                    document.record_strict_invariant();
                    handleWarning(strict_invariant(inv));
                }
            }
        }
    }
    if (!loc.exp_rate.empty()) {
        if (auto& expr = loc.exp_rate; checkExpression(expr)) {
            if (!is_integral(expr) && expr.get_kind() != FRACTION && !expr.get_type().is_double()) {
                handleError(number_expected(expr));
            }
        }
    }
    if (loc.uid.get_name() == "__RESET__")
        handleWarning(deprecated_reset(loc.uid));
}

void TypeChecker::visit_edge(Edge& edge)
{
    DocumentVisitor::visit_edge(edge);

    // select
    for (auto& symbol : edge.select)
        checkType(symbol.get_type());

    // guard
    bool strictBound = false;
    if (!edge.guard.empty()) {
        if (checkExpression(edge.guard)) {
            if (!is_guard(edge.guard))
                handleError(cannot_be_used_as_guard(edge.guard));
            else if (edge.guard.changes_any_variable())
                handleError(must_be_side_effect_free(edge.guard));
            if (hasStrictLowerBound(edge.guard)) {
                if (edge.control) {
                    document.record_strict_lower_bound_on_controllable_edges();
                }
                strictBound = true;
            }
            if (hasStrictUpperBound(edge.guard)) {
                strictBound = true;
            }
        }
    }

    // sync
    if (!edge.sync.empty()) {
        if (checkExpression(edge.sync)) {
            const Type& channel = edge.sync.get(0).get_type();
            if (!channel.is_channel())
                handleError(channel_expected(edge.sync.get(0)));
            else if (edge.sync.changes_any_variable())
                handleError(must_be_side_effect_free(edge.sync));
            else {
                const bool hasClockGuard = !edge.guard.empty() && !is_integral(edge.guard);
                const bool isUrgent = channel.is(URGENT);
                const bool receivesBroadcast = channel.is(BROADCAST) && edge.sync.get_sync() == SYNC_QUE;

                if (isUrgent && hasClockGuard) {
                    document.set_urgent_transition();
                    handleWarning(clock_guards_not_allowed_on_urgent_edges(edge.sync));
                } else if (receivesBroadcast && hasClockGuard) {
                    document.clock_guard_recv_broadcast();
                    /*
                      This is now allowed, though it is expensive.

                    handle_error(edge.sync,
                                "$Clock_guards_are_not_allowed_on_broadcast_receivers");
                    */
                }
                if (receivesBroadcast && edge.guard.is_true()) {
                    if (edge.dst == nullptr)  // dst is null at least in a case of branchpoint
                        handleWarning(input_edges_must_be_deterministic(edge.sync));
#ifndef NDEBUG
                    else if (!edge.dst->invariant.is_true()) {
                        // not handled correctly by the engine and it is expensive to fix.
                        handleWarning(may_need_guard_involving_target_invariant(edge.sync));
                    }
                    /*
                      The warning above gives too many false alarms and is therefore disabled.
                      In particular, it does not consider the common idiom of clock reset (i.e. guard
                      is irrelevant). Details: the above case may lead to violation of target
                      invariant if unchecked, however the invariant *is* being checked in the engine
                      and halts with "violates model sanity with transition" + proper diagnostics
                      about the transition and location.
                    */
#endif /* NDEBUG */
                }
                if (isUrgent && strictBound)
                    handleWarning(strict_bounds_on_urgent_edges(edge.guard));
            }

            switch (syncUsed) {
            case 0:
                switch (edge.sync.get_sync()) {
                case SYNC_BANG:
                case SYNC_QUE: syncUsed = 1; break;
                case SYNC_CSP: syncUsed = 2; break;
                }
                break;
            case 1:
                switch (edge.sync.get_sync()) {
                case SYNC_BANG:
                case SYNC_QUE:
                    // ok
                    break;
                case SYNC_CSP: syncUsed = -1; break;
                }
                break;
            case 2:
                switch (edge.sync.get_sync()) {
                case SYNC_BANG:
                case SYNC_QUE: syncUsed = -1; break;
                case SYNC_CSP:
                    // ok
                    break;
                }
                break;
            default:
                // nothing
                ;
            }
            if (syncUsed == -1)
                handleError(csp_and_io_cannot_be_mixed(edge.sync));

            if (refinementWarnings) {
                switch (edge.sync.get_sync()) {
                case SYNC_BANG:
                    if (edge.control)
                        handleWarning(outputs_should_be_uncontrollable(edge.sync));
                    break;
                case SYNC_QUE:
                    if (!edge.control)
                        handleWarning(inputs_should_be_controllable(edge.sync));
                    break;
                default: handleWarning(csp_sync_is_incompatible_with_refinement_checking(edge.sync));
                }
            }
        }
    }

    // assignment
    checkAssignmentExpression(edge.assign);

    // probability
    if (!edge.prob.empty()) {
        if (checkExpression(edge.prob)) {
            if (!is_probability(edge.prob))
                handleError(cannot_be_used_as_probability(edge.prob));
            else if (edge.prob.changes_any_variable())
                handleError(must_be_side_effect_free(edge.prob));
        }
    }
}

void TypeChecker::visit_instance_line(LSCInstanceLine& instance) { DocumentVisitor::visit_instance_line(instance); }

void TypeChecker::visit_message(LSCMessage& message)
{
    DocumentVisitor::visit_message(message);

    if (!message.label.empty()) {
        if (checkExpression(message.label)) {
            const Type& channel = message.label.get(0).get_type();
            if (!channel.is_channel())
                handleError(channel_expected(message.label.get(0)));
            else if (message.label.changes_any_variable())
                handleError(must_be_side_effect_free(message.label));
        }
    }
}
void TypeChecker::visit_condition(LSCCondition& condition)
{
    DocumentVisitor::visit_condition(condition);
    if (!condition.label.empty()) {
        if (checkExpression(condition.label)) {
            if (!is_guard(condition.label))
                handleError(cannot_be_used_as_condition(condition.label));
            else if (condition.label.changes_any_variable())
                handleError(must_be_side_effect_free(condition.label));
        }
    }
}
void TypeChecker::visit_update(LSCUpdate& update)
{
    DocumentVisitor::visit_update(update);
    if (!update.label.empty()) {
        checkAssignmentExpression(update.label);
    }
}

void TypeChecker::visit_progress(Progress& progress)
{
    checkExpression(progress.guard);
    checkExpression(progress.measure);
    if (!progress.guard.empty() && !is_integral(progress.guard))
        handleError(progress_guard_must_evaluate_to_boolean(progress.guard));
    if (!is_integral(progress.measure))
        handleError(progress_measure_must_evaluate_to_integer(progress.measure));
}

void TypeChecker::visit_gantt(GanttEntry& gc)
{
    for (auto& param : gc.parameters)
        checkType(param.get_type());
    for (auto& map : gc.mapping) {
        for (auto& param : map.parameters)
            checkType(param.get_type());
        checkExpression(map.predicate);
        if (!is_integral(map.predicate) && !is_constraint(map.predicate))
            handleError(boolean_expected(map.predicate));
        checkExpression(map.mapping);
        if (!is_integral(map.mapping))
            handleError(integer_expected(map.mapping));
    }
}

void TypeChecker::visit_instance(Instance& instance)
{
    DocumentVisitor::visit_instance(instance);

    // Check the parameters of the instance.
    const Type& type = instance.uid.get_type();
    for (uint32_t i = 0; i < type.size(); i++)
        checkType(type[i]);

    // Check arguments.
    for (uint32_t i = type.size(); i < type.size() + instance.arguments; i++) {
        const Symbol& parameter = instance.parameters[i];
        Expression argument = instance.mapping[parameter];

        if (!checkExpression(argument)) {
            continue;
        }

        // For template instantiation, the argument must be side-effect free
        if (argument.changes_any_variable()) {
            handleError(must_be_side_effect_free(argument));
            continue;
        }

        // We have three ok cases:
        // - Value parameter with computable argument
        // - Constant reference with computable argument
        // - Reference parameter with unique lhs argument
        // If none of the cases are true, then we generate an error
        const bool ref = parameter.get_type().is(REF);
        const bool constant = parameter.get_type().is_constant();
        const bool computable = isCompileTimeComputable(argument);

        if ((!ref && !computable) || (ref && !constant && !isUniqueReference(argument)) ||
            (ref && constant && !computable)) {
            handleError(incompatible_argument(argument));
            continue;
        }

        checkParameterCompatible(parameter.get_type(), argument);
    }
}

void TypeChecker::visitProperty(Expression& expr)
{
    if (checkExpression(expr)) {
        if (expr.changes_any_variable()) {
            handleError(must_be_side_effect_free(expr));
        }
        if (expr.get_kind() == LOAD_STRAT || expr.get_kind() == SAVE_STRAT) {
            if (!expr.get(0).get_type().is_string())
                handleError(loadStrategy_and_saveStrategy_expect_string(expr));
            return;
        }
        if (!is_formula(expr))
            handleError(must_be_valid_formula(expr));
        if (is_game_property(expr)) {
            /*
            for (uint32_t i = 0; i < expr.get_size(); i++)
            {
                if (is_formula(expr[i]))
                {
                    for (uint32_t j = 0; j < expr[i].get_size(); j++)
                    {
                        if (!is_constraint(expr[i][j]))
                        {
                            handle_error(expr[i][j], "$Nesting_of_path_quantifiers_is_not_allowed");
                        }
                    }
                }
            }
            */
        } else if (auto k = expr.get_kind(); k != SUP_VAR && k != INF_VAR && k != BOUNDS_VAR && k != SCENARIO &&
                                             k != PROBA_MIN_BOX && k != PROBA_MIN_DIAMOND && k != PROBA_BOX &&
                                             k != PROBA_DIAMOND && k != PROBA_EXP && k != PROBA_CMP && k != SIMULATE &&
                                             k != SIMULATEREACH && k != MITL_FORMULA &&
                                             k != MIN_EXP &&  // ALREADY CHECKED IN PARSE
                                             k != MAX_EXP)    // ALREADY CHECKED IN PARSE
        {
            for (uint32_t i = 0; i < expr.get_size(); i++) {
                // No nesting except for constraints
                if (!is_constraint(expr[i]))
                    handleError(nested_path_quntifiers_not_supported(expr[i]));
            }
        }
        if (expr.get_kind() == PO_CONTROL) {
            /* Observations on clock constraints are limited to be
             * weak for lower bounds and strict for upper bounds.
             */
            checkObservationConstraints(expr);
        }
        if (has_MITL_in_quantified_sub(expr) && expr.get_kind() != MITL_FORMULA)
            handleError(mitl_inside_forall_or_exists_in_non_mitl(expr));
    }
}

/**
 * Checks that \a expr is a valid assignment expression. Errors or
 * warnings are issued via calls to handle_error() and
 * handle_warning(). Returns true if no errors were issued, false
 * otherwise.
 *
 * An assignment expression is any:
 *
 *  - expression of an expression statement,
 *
 *  - initialisation or step expression in a for-clause
 *
 *  - expression in the update field of an edge
 *
 *  - expression in the label field of an update (LSC)
 */
bool TypeChecker::checkAssignmentExpression(Expression& expr)
{
    if (!checkExpression(expr)) {
        return false;
    }

    if (!is_assignable(expr.get_type()) && !is_void(expr)) {
        handleError(invalid_assignment(expr));
        return false;
    }

    if (expr.get_kind() != FUN_CALL_EXT && (expr.get_kind() != CONSTANT || expr.get_value() != 1)) {
        checkIgnoredValue(expr);
    }

    return true;
}

/** Checks that the expression can be used as a condition (e.g. for if). */
bool TypeChecker::checkConditionalExpressionInFunction(const Expression& expr)
{
    if (!(is_integral(expr) || is_constraint(expr))) {
        handleError(boolean_expected(expr));
        return false;
    }
    return true;
}

void TypeChecker::checkObservationConstraints(const Expression& expr)
{
    for (uint32_t i = 0; i < expr.get_size(); ++i) {
        checkObservationConstraints(expr[i]);
    }

    bool invalid = false;

    switch (expr.get_kind()) {
    case LT:  // int < clock
    case GE:  // int >= clock
        invalid = is_integral(expr[0]) && is_clock(expr[1]);
        break;

    case LE:  // clock <= int
    case GT:  // clock > int
        invalid = is_clock(expr[0]) && is_integral(expr[1]);
        break;

    case EQ:   // clock == int || int == clock
    case NEQ:  // clock != int || int != clock
        invalid = (is_clock(expr[0]) && is_integral(expr[1])) || (is_integral(expr[0]) && is_clock(expr[1]));
        break;

    default:;
    }

    if (invalid)
        handleError(clock_lower_and_upper_bounds(expr));
    else {
        switch (expr.get_kind())  // No clock differences.
        {
        case LT:
        case LE:
        case GT:
        case GE:
        case EQ:
        case NEQ:
            if ((is_clock(expr[0]) && is_clock(expr[1])) || (is_diff(expr[0]) && is_integer(expr[1])) ||
                (is_integer(expr[0]) && is_diff(expr[1]))) {
                handleError(clock_difference_is_not_supported(expr));
            }
            break;

        default:;
        }
    }
}

void TypeChecker::visit_function(Function& fn)
{
    DocumentVisitor::visit_function(fn);
    /* Check that the return type is consistent and is a valid return
     * type.
     */
    const Type& return_type = fn.uid.get_type()[0];
    checkType(return_type);
    if (!return_type.is_void() && !valid_return_type(return_type))
        handleError(invalid_return_type(return_type));

    /* Type check the function body: Type checking return statements
     * requires access to the return type, hence we store a pointer to
     * the current function being type checked in the \a function
     * member.
     */
    function = &fn;
    fn.body->accept(*this);
    function = nullptr;

    /* Check if there are dynamic expressions in the function body*/
    checkDynamicExpressions(*fn.body);

    /* Collect identifiers of things external to the function accessed
     * or changed by the function. Notice that neither local variables
     * nor parameters are considered to be changed or accessed by a
     * function.
     */
    fn.changes = collect_changes(*fn.body);
    fn.depends = collect_dependencies(*fn.body);

    for (const auto& var : fn.variables) {
        fn.changes.erase(var.uid);
        fn.depends.erase(var.uid);
    }
    const uint32_t parameters = fn.uid.get_type().size() - 1;
    for (uint32_t i = 0; i < parameters; i++) {
        fn.changes.erase(fn.body->get_frame()[i]);
        fn.depends.erase(fn.body->get_frame()[i]);
    }
}

int32_t TypeChecker::visit_empty_statement(EmptyStatement&) { return 0; }

int32_t TypeChecker::visit_expr_statement(ExprStatement& stat)
{
    checkAssignmentExpression(stat.expr);
    return 0;
}

int32_t TypeChecker::visit_assert_statement(AssertStatement& stat)
{
    if (checkExpression(stat.expr) && stat.expr.changes_any_variable())
        handleError(must_be_side_effect_free(stat.expr));
    return 0;
}

int32_t TypeChecker::visit_for_statement(ForStatement& stat)
{
    checkAssignmentExpression(stat.init);

    if (checkExpression(stat.cond)) {
        checkConditionalExpressionInFunction(stat.cond);
    }

    checkAssignmentExpression(stat.step);

    return stat.stat->accept(*this);
}

int32_t TypeChecker::visit_iteration_statement(RangeStatement& stat)
{
    const Type& type = stat.symbol.get_type();
    checkType(type);

    /* We only support iteration over scalars and integers.
     */
    if (!type.is_scalar() && !type.is_integer())
        handleError(scalar_or_integer_expected(type));
    else if (!type.is(RANGE))
        handleError(range_expected(type));

    return stat.stat->accept(*this);
}

int32_t TypeChecker::visit_while_statement(WhileStatement& stat)
{
    if (checkExpression(stat.cond))
        checkConditionalExpressionInFunction(stat.cond);
    return stat.stat->accept(*this);
}

int32_t TypeChecker::visit_do_while_statement(DoWhileStatement& stat)
{
    if (checkExpression(stat.cond))
        checkConditionalExpressionInFunction(stat.cond);
    return stat.stat->accept(*this);
}

int32_t TypeChecker::visit_block_statement(BlockStatement& stat)
{
    /* Check type and initialiser of local variables (parameters are
     * also considered local variables).
     */
    auto frame = stat.get_frame();
    for (uint32_t i = 0; i < frame.get_size(); ++i) {
        Symbol symbol = frame[i];
        checkType(symbol.get_type());
        if (auto* d = symbol.get_data(); d != nullptr) {
            auto* var = static_cast<Variable*>(d);
            if (!var->init.empty() && checkExpression(var->init)) {
                if (var->init.changes_any_variable()) {
                    /* This is stronger than C. However side-effects in
                     * initialisers are nasty: For records, the evaluation
                     * order may be different from the order in the input
                     * file.
                     */
                    handleError(must_be_side_effect_free(var->init));
                } else {
                    var->init = checkInitialiser(symbol.get_type(), var->init);
                }
            }
        }
    }

    /* Check statements.
     */
    for (const auto& s : stat)
        s->accept(*this);
    return 0;
}

int32_t TypeChecker::visit_if_statement(IfStatement& stat)
{
    if (checkExpression(stat.cond)) {
        checkConditionalExpressionInFunction(stat.cond);
    }
    stat.trueCase->accept(*this);
    if (stat.falseCase) {
        stat.falseCase->accept(*this);
    }
    return 0;
}

int32_t TypeChecker::visit_return_statement(ReturnStatement& stat)
{
    if (!stat.value.empty()) {
        checkExpression(stat.value);

        /* The only valid return types are integers and records. For these
         * two types, the type rules are the same as for parameters.
         */
        const Type& return_type = function->uid.get_type()[0];
        checkParameterCompatible(return_type, stat.value);
    }
    return 0;
}

/**
 * Returns true iff argument type is compatible with parameter type.
 */
bool TypeChecker::isParameterCompatible(const Type& paramType, const Expression& arg) const
{
    const bool ref = paramType.is(REF);
    const bool constant = paramType.is_constant();
    const bool lvalue = isModifiableLValue(arg);
    const Type& argType = arg.get_type();
    // For non-const reference parameters, we require a modifiable
    // lvalue argument
    if (ref && !constant && !lvalue) {
        return false;
    }

    if (paramType.is_channel() && argType.is_channel()) {
        return argType.channel_capability() >= paramType.channel_capability();
    } else if (ref && lvalue) {
        return argType.is_equivalent(paramType);
    } else {
        return paramType.is_assignment_compatible(argType);
    }
}

/**
 * Checks whether argument type is compatible with parameter type.
 */
bool TypeChecker::checkParameterCompatible(const Type& paramType, const Expression& arg)
{
    if (!isParameterCompatible(paramType, arg)) {
        handleError(incompatible_argument(arg));
        return false;
    }
    return true;
}

/**
 * Checks whether init is a valid initialiser for a variable or
 * constant of the given type. For record types, the initialiser is
 * reordered to fit the order of the fields and the new initialiser is
 * returned. REVISIT: Can a record initialiser have side-effects? Then
 * such reordering is not valid.
 */
Expression TypeChecker::checkInitialiser(const Type& type, const Expression& init)
{
    if (type.is_assignment_compatible(init.get_type(), true)) {
        return init;
    } else if (type.is_array() && init.get_kind() == LIST) {
        auto subtype = type.get_sub();
        auto result = std::vector<Expression>(init.get_size());
        for (uint32_t i = 0; i < init.get_type().size(); i++) {
            if (!init.get_type().get_label(i).empty())
                handleError(field_name_not_allowed_in_array_init(init[i]));
            checkInitialiser(subtype, init[i]);
        }
        return Expression::create_nary(LIST, result, init.get_position(), type);
    } else if (type.is_record() && init.get_kind() == LIST) {
        /* In order to access the record labels we have to strip any
         * prefixes and labels from the record type.
         */
        auto result = std::vector<Expression>(type.get_record_size());
        auto current = uint32_t{0};
        for (uint32_t i = 0; i < init.get_type().size(); ++i, ++current) {
            const std::string& label = init.get_type().get_label(i);
            if (!label.empty()) {
                auto index = type.find_index_of(label);
                if (!index) {
                    handleError(unknown_field_name(init[i]));
                    break;
                } else {
                    current = *index;
                }
            }
            if (current >= type.get_record_size()) {
                handleError(too_many_elements_in_initializer(init[i]));
                break;
            }
            if (!result[current].empty()) {
                handleError(multiple_initializers_for_field(init[i]));
                continue;
            }
            result[current] = checkInitialiser(type.get_sub(current), init[i]);
        }
        // Check that all fields do have an initializer.
        for (const auto& res : result) {
            if (res.empty()) {
                handleError(incomplete_initializer(init));
                break;
            }
        }
        return Expression::create_nary(LIST, result, init.get_position(), type);
    }
    handleError(invalid_initializer(init));
    return init;
}

Type TypeChecker::getInlineIfCommonType(const Type& t1, const Type& t2) const
{
    if (t1.is_record())
        return t1;
    else if (t2.is_record())
        return t2;
    else if ((t1.is_clock() && !t2.is_clock()) || (!t1.is_clock() && t2.is_clock()))
        return Type{DOUBLE, {}, 0};
    else if (t1.is_assignment_compatible(t2))
        return t1;
    else if (t2.is_assignment_compatible(t1))
        return t2;
    else if (t2.is_equivalent(t1))
        return t1;
    else
        return Type{};
}

/** Type check and checkExpression the expression. This function performs
    basic type checking of the given expression and assigns a type to
    every subexpression of the expression. It checks that only
    left-hand side values are updated, checks that functions are
    called with the correct arguments, checks that operators are used
    with the correct operands and checks that operands to assignment
    operators are assignment compatible. Errors are reported by
    calling handle_error(). This function does not check/compute the
    range of integer expressions and thus does not produce
    out-of-range errors or warnings. Returns true if no type errors
    were found, false otherwise.
*/
bool TypeChecker::checkExpression(Expression& expr)
{
    /* Do not check empty expressions.
     */
    if (expr.empty())
        return true;

    /* CheckExpression sub-expressions.
     */
    bool ok = true;
    for (uint32_t i = 0; i < expr.get_size(); i++)
        ok &= checkExpression(expr[i]);

    /* Do not checkExpression the expression if any of the sub-expressions
     * contained errors.
     */
    if (!ok)
        return false;

    /* CheckExpression the expression. This depends on the kind of expression
     * we are dealing with.
     */
    Type type;
    // Type arg1;
    // Type arg2;
    // Type arg3;
    switch (expr.get_kind()) {
        // It is possible to have DOT expressions as data.x
        // with data being an array of struct. The type checker
        // is broken and trying
        // expr[0].get_type().is_record() or
        // expr[0].is_process() cannot detect this.
        // This should be fixed one day.
        /*
    case DOT:
        if (expr[0].get_type().is_process(Constants::PROCESS) ||
            expr[0].get_type().is(Constants::RECORD))
        {
            return true;
        }
        else
        {
            handle_error(expr, "$Invalid_type");
            return false;
        }
        */
    case SUM_DYNAMIC:
        if (is_integral(expr[2]) || is_double_value(expr[2])) {
            type = expr[2].get_type();
        } else if (is_invariant(expr[2]) || is_guard(expr[2])) {
            type = Type::create_primitive(Constants::DOUBLE_INV_GUARD);
        } else {
            handleError(invalid_sum(expr));
            return false;
        }
        break;
    case FRACTION:
        if (is_integral(expr[0]) && is_integral(expr[1]))
            type = Type::create_primitive(Constants::FRACTION);
        break;

    case PLUS:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::INT);
        } else if ((is_integer(expr[0]) && is_clock(expr[1])) || (is_clock(expr[0]) && is_integer(expr[1]))) {
            type = Type::create_primitive(CLOCK);
        } else if ((is_diff(expr[0]) && is_integer(expr[1])) || (is_integer(expr[0]) && is_diff(expr[1]))) {
            type = Type::create_primitive(DIFF);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            // SMC extension.
            type = Type::create_primitive(Constants::DOUBLE);
        }
        break;

    case MINUS:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::INT);
        } else if (is_clock(expr[0]) && is_integer(expr[1]))
        // removed  "|| is_integer(expr[0].type) && is_clock(expr[1].type)"
        // in order to be able to convert into ClockGuards
        {
            type = Type::create_primitive(CLOCK);
        } else if ((is_diff(expr[0]) && is_integer(expr[1])) || (is_integer(expr[0]) && is_diff(expr[1])) ||
                   (is_clock(expr[0]) && is_clock(expr[1]))) {
            type = Type::create_primitive(DIFF);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            // SMC extension.
            // x-y with that semantic should be written x+(-y)
            type = Type::create_primitive(Constants::DOUBLE);
        }
        break;

    case AND:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if (is_invariant(expr[0]) && is_invariant(expr[1])) {
            type = Type::create_primitive(INVARIANT);
        } else if (is_invariant_wr(expr[0]) && is_invariant_wr(expr[1])) {
            type = Type::create_primitive(INVARIANT_WR);
        } else if (is_guard(expr[0]) && is_guard(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_constraint(expr[0]) && is_constraint(expr[1])) {
            type = Type::create_primitive(CONSTRAINT);
        } else if (is_formula(expr[0]) && is_formula(expr[1])) {
            type = Type::create_primitive(FORMULA);
        }
        break;

    case OR:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if (is_integral(expr[0]) && is_invariant(expr[1])) {
            type = Type::create_primitive(INVARIANT);
        } else if (is_invariant(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(INVARIANT);
        } else if (is_integral(expr[0]) && is_invariant_wr(expr[1])) {
            type = Type::create_primitive(INVARIANT_WR);
        } else if (is_invariant_wr(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(INVARIANT_WR);
        } else if (is_integral(expr[0]) && is_guard(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_guard(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_constraint(expr[0]) && is_constraint(expr[1])) {
            type = Type::create_primitive(CONSTRAINT);
        }
        break;

    case XOR:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        }
        break;

    case SPAWN: {
        Template* temp = document.find_dynamic_template(expr[0].get_symbol().get_name());
        if (temp == nullptr) {
            handleError(cannot_spawn_a_non_dynamic_template(expr));
            return false;
        }
        if (temp->parameters.get_size() != expr.get_size() - 1) {
            handleError(wrong_number_of_arguments(expr));
            return false;
        }
        for (uint32_t i = 0; i < temp->parameters.get_size(); i++) {
            if (!checkSpawnParameterCompatible(temp->parameters[i].get_type(), expr[i + 1])) {
                return false;
            }
        }

        /* check that spawn is only made for templates with definitions*/
        if (!temp->is_defined) {
            handleError(template_only_declared_and_undefined(expr));
            return false;
        }
        type = Type::create_primitive(Constants::INT);
        break;
    }

    case NUMOF: {
        if (document.find_dynamic_template(expr[0].get_symbol().get_name()) != nullptr) {
            type = Type::create_primitive(Constants::INT);
        } else {
            handleError(not_dynamic_template(expr));
            return false;
        }
        break;
    }

    case EXIT: {
        assert(temp);
        if (!temp->dynamic) {
            handleError(exit_only_in_dynamic_template(expr));
            return false;
        }

        type = Type::create_primitive(Constants::INT);

        break;
    }

    case LT:
    case LE:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if ((is_clock(expr[0]) && is_clock(expr[1])) || (is_clock(expr[0]) && is_bound(expr[1])) ||
                   (is_clock(expr[1]) && is_bound(expr[0])) || (is_diff(expr[0]) && is_bound(expr[1])) ||
                   (is_bound(expr[0]) && is_diff(expr[1]))) {
            type = Type::create_primitive(INVARIANT);
        } else if (is_number(expr[0]) && is_clock(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_clock(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        }
        break;

    case EQ:
        // FIXME: Apparently the case cost == expr goes through, which is obviously not good.

        if ((is_clock(expr[0]) && is_clock(expr[1])) || (is_clock(expr[0]) && is_number(expr[1])) ||
            (is_number(expr[0]) && is_clock(expr[1])) || (is_diff(expr[0]) && is_number(expr[1])) ||
            (is_number(expr[0]) && is_diff(expr[1]))) {
            type = Type::create_primitive(GUARD);
        } else if (expr[0].get_type().is_equality_compatible(expr[1].get_type())) {
            type = Type::create_primitive(Constants::BOOL);
        } else if ((expr[0].get_type().is(RATE) && (is_integral(expr[1]) || is_double_value(expr[1]))) ||
                   ((is_integral(expr[0]) || is_double_value(expr[0])) && expr[1].get_type().is(RATE))) {
            type = Type::create_primitive(INVARIANT_WR);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        }
        break;

    case NEQ:
        if (expr[0].get_type().is_equality_compatible(expr[1].get_type())) {
            type = Type::create_primitive(Constants::BOOL);
        } else if ((is_clock(expr[0]) && is_clock(expr[1])) || (is_clock(expr[0]) && is_integer(expr[1])) ||
                   (is_integer(expr[0]) && is_clock(expr[1])) || (is_diff(expr[0]) && is_integer(expr[1])) ||
                   (is_integer(expr[0]) && is_diff(expr[1]))) {
            type = Type::create_primitive(CONSTRAINT);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        }
        break;

    case GE:
    case GT:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if ((is_clock(expr[0]) && is_clock(expr[1])) || (is_integer(expr[0]) && is_clock(expr[1])) ||
                   (is_integer(expr[1]) && is_clock(expr[0])) || (is_diff(expr[0]) && is_integer(expr[1])) ||
                   (is_integer(expr[0]) && is_diff(expr[1]))) {
            type = Type::create_primitive(INVARIANT);
        } else if (is_number(expr[0]) && is_clock(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_clock(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        }
        break;

    case MULT:
    case DIV:
    case POW:
    case MIN:
    case MAX:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::INT);
        } else if (is_number(expr[0]) && is_number(expr[1])) {
            type = Type::create_primitive(Constants::DOUBLE);
        }
        break;

    case MOD:
    case BIT_AND:
    case BIT_OR:
    case BIT_XOR:
    case BIT_LSHIFT:
    case BIT_RSHIFT:
        if (is_integral(expr[0]) && is_integral(expr[1])) {
            type = Type::create_primitive(Constants::INT);
        }
        break;

    case NOT:
        if (is_integral(expr[0])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if (is_constraint(expr[0])) {
            type = Type::create_primitive(CONSTRAINT);
        }
        break;

    case UNARY_MINUS:
        if (is_integral(expr[0])) {
            type = Type::create_primitive(Constants::INT);
        } else if (is_number(expr[0])) {
            type = Type::create_primitive(Constants::DOUBLE);
        }
        break;

    case RATE:
        if (is_cost(expr[0]) || is_clock(expr[0])) {
            type = Type::create_primitive(RATE);
        }
        break;

    case ASSIGN:
        if (!expr[0].get_type().is_assignment_compatible(expr[1].get_type())) {
            handleError(incompatible_types(expr));
            return false;
        } else if (!isModifiableLValue(expr[0])) {
            handleError(lvalue_expected(expr[0]));
            return false;
        }
        type = expr[0].get_type();
        break;

    case ASS_PLUS:
        if ((!is_integer(expr[0]) && !is_cost(expr[0])) || !is_integral(expr[1]))
            handleError(increment_only_integers_and_cost(expr));
        else if (!isModifiableLValue(expr[0]))
            handleError(lvalue_expected(expr[0]));
        type = expr[0].get_type();
        break;

    case ASS_MINUS:
    case ASS_DIV:
    case ASS_MOD:
    case ASS_MULT:
    case ASS_AND:
    case ASS_OR:
    case ASS_XOR:
    case ASS_LSHIFT:
    case ASS_RSHIFT:
        if (!is_integral(expr[0]) || !is_integral(expr[1])) {
            handleError(use_regular_assignment_on_non_integer(expr));
            return false;
        } else if (!isModifiableLValue(expr[0])) {
            handleError(lvalue_expected(expr[0]));
            return false;
        }
        type = expr[0].get_type();
        break;

    case POST_INCREMENT:
    case PRE_INCREMENT:
    case POST_DECREMENT:
    case PRE_DECREMENT:
        if (!isModifiableLValue(expr[0])) {
            handleError(lvalue_expected(expr[0]));
            return false;
        } else if (!is_integer(expr[0])) {
            handleError(integer_expected(expr));
            return false;
        }
        type = Type::create_primitive(Constants::INT);
        break;

    case FMA_F:
    case RANDOM_TRI_F:
        if (!is_number(expr[2])) {
            handleError(number_expected(expr[2]));
            return false;
        }  // fall-through to check the second argument:
        [[fallthrough]];
    case FMOD_F:
    case FMAX_F:
    case FMIN_F:
    case FDIM_F:
    case POW_F:
    case HYPOT_F:
    case ATAN2_F:
    case NEXT_AFTER_F:
    case COPY_SIGN_F:
    case RANDOM_ARCSINE_F:
    case RANDOM_BETA_F:
    case RANDOM_GAMMA_F:
    case RANDOM_NORMAL_F:
    case RANDOM_WEIBULL_F:
        if (!is_number(expr[1])) {
            handleError(number_expected(expr[1]));
            return false;
        }  // fall-through to check the first argument:
        [[fallthrough]];
    case FABS_F:
    case EXP_F:
    case EXP2_F:
    case EXPM1_F:
    case LN_F:
    case LOG_F:
    case LOG10_F:
    case LOG2_F:
    case LOG1P_F:
    case SQRT_F:
    case CBRT_F:
    case SIN_F:
    case COS_F:
    case TAN_F:
    case ASIN_F:
    case ACOS_F:
    case ATAN_F:
    case SINH_F:
    case COSH_F:
    case TANH_F:
    case ASINH_F:
    case ACOSH_F:
    case ATANH_F:
    case ERF_F:
    case ERFC_F:
    case TGAMMA_F:
    case LGAMMA_F:
    case CEIL_F:
    case FLOOR_F:
    case TRUNC_F:
    case ROUND_F:
    case LOGB_F:
    case RANDOM_F:
    case RANDOM_POISSON_F:
        if (!is_number(expr[0])) {
            handleError(number_expected(expr[0]));
            return false;
        }
        type = Type::create_primitive(Constants::DOUBLE);
        break;

    case LDEXP_F:
        if (!is_integral(expr[1])) {
            handleError(integer_expected(expr[1]));
            return false;
        }
        if (!is_number(expr[0])) {
            handleError(number_expected(expr[0]));
            return false;
        }
        type = Type::create_primitive(Constants::DOUBLE);
        break;

    case ABS_F:
    case FP_CLASSIFY_F:
        if (!is_integral(expr[0])) {
            handleError(integer_expected(expr[0]));
            return false;
        }
        type = Type::create_primitive(Constants::INT);
        break;

    case ILOGB_F:
    case FINT_F:
        if (!is_number(expr[0])) {
            handleError(number_expected(expr[0]));
            return false;
        }
        type = Type::create_primitive(Constants::INT);
        break;

    case IS_FINITE_F:
    case IS_INF_F:
    case IS_NAN_F:
    case IS_NORMAL_F:
    case SIGNBIT_F:
    case IS_UNORDERED_F:
        if (!is_number(expr[0])) {
            handleError(number_expected(expr[0]));
            return false;
        }
        type = Type::create_primitive(Constants::BOOL);
        break;

    case INLINE_IF:
        if (!(is_integral(expr[0]) || is_guard(expr[0]))) {
            handleError(first_argument_of_inline_if_must_be_integer(expr));
            return false;
        }

        type = getInlineIfCommonType(expr[1].get_type(), expr[2].get_type());
        if (!type.is_inline_if_compatible(expr[1].get_type(), expr[2].get_type())) {
            handleError(incompatible_arguments_to_inline_if(expr));
            return false;
        }
        break;

    case COMMA:
        if (!is_assignable(expr[0].get_type()) && !is_void(expr[0])) {
            handleError(incompatible_type_for_comma(expr[0]));
            return false;
        }
        if (!is_assignable(expr[1].get_type()) && !is_void(expr[1])) {
            handleError(incompatible_type_for_comma(expr[1]));
            return false;
        }
        checkIgnoredValue(expr[0]);
        type = expr[1].get_type();
        break;

    case FUN_CALL:
    case FUN_CALL_EXT: {
        checkExpression(expr[0]);

        bool result = true;
        const Type& t = expr[0].get_type();
        const uint32_t parameters = t.size() - 1;
        for (uint32_t i = 0; i < parameters; i++) {
            const Type& parameter = t[i + 1];
            const Expression& argument = expr[i + 1];
            result &= checkParameterCompatible(parameter, argument);
        }
        return result;
    }

    case ARRAY: {
        const Type& arg1 = expr[0].get_type();
        const Type& arg2 = expr[1].get_type();

        // The left side must be an array.
        if (!arg1.is_array()) {
            handleError(array_expected(expr[0]));
            return false;
        }
        type = arg1.get_sub();

        // Check the type of the index.
        if (arg1.get_array_size().is_integer() && arg2.is_integral()) {
        } else if (arg1.get_array_size().is_scalar() && arg2.is_scalar()) {
            if (!arg1.get_array_size().is_same_scalar(arg2)) {
                handleError(incompatible_type(expr[1]));
                return false;
            }
        } else {
            handleError(incompatible_type(expr[1]));
        }
        break;
    }
    case FORALL:
        compileTimeComputableValues.add_symbol(expr[0].get_symbol());
        checkType(expr[0].get_symbol().get_type());

        if (is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if (is_invariant(expr[1])) {
            type = Type::create_primitive(INVARIANT);
        } else if (is_invariant_wr(expr[1])) {
            type = Type::create_primitive(INVARIANT_WR);
        } else if (is_guard(expr[1])) {
            type = Type::create_primitive(GUARD);
        } else if (is_constraint(expr[1])) {
            type = Type::create_primitive(CONSTRAINT);
        } else {
            handleError(boolean_expected(expr[1]));
        }

        if (expr[1].changes_any_variable()) {
            handleError(must_be_side_effect_free(expr[1]));
        }
        break;

    case EXISTS:
        compileTimeComputableValues.add_symbol(expr[0].get_symbol());
        checkType(expr[0].get_symbol().get_type());

        if (is_integral(expr[1])) {
            type = Type::create_primitive(Constants::BOOL);
        } else if (is_constraint(expr[1])) {
            type = Type::create_primitive(CONSTRAINT);
        } else {
            handleError(boolean_expected(expr[1]));
        }

        if (expr[1].changes_any_variable()) {
            handleError(must_be_side_effect_free(expr[1]));
        }
        break;

    case SUM:
        compileTimeComputableValues.add_symbol(expr[0].get_symbol());
        checkType(expr[0].get_symbol().get_type());

        if (is_integral(expr[1])) {
            type = Type::create_primitive(Constants::INT);
        } else if (is_number(expr[1])) {
            type = Type::create_primitive(Constants::DOUBLE);
        } else {
            handleError(number_expected(expr[1]));
        }

        if (expr[1].changes_any_variable()) {
            handleError(must_be_side_effect_free(expr[1]));
        }
        break;

    case AF:
    case AG:
    case EF:
    case EG:
    case EF_CONTROL:
    case CONTROL:
    case CONTROL_TOPT:
    case CONTROL_TOPT_DEF1:
    case CONTROL_TOPT_DEF2:
    case PMAX:
        if (is_formula(expr[0])) {
            type = Type::create_primitive(FORMULA);
        }
        break;

    case PO_CONTROL:
        if (is_formula_list(expr[0]) && is_formula(expr[1])) {
            type = Type::create_primitive(FORMULA);
        }
        break;

    case LEADS_TO:
    case SCENARIO2:
    case A_UNTIL:
    case A_WEAK_UNTIL:
    case A_BUCHI:
        if (is_formula(expr[0]) && is_formula(expr[1])) {
            type = Type::create_primitive(FORMULA);
        }
        break;

    case SCENARIO: type = Type::create_primitive(FORMULA); break;

    case SIMULATEREACH:
    case SIMULATE: {
        if (!is_const_integer(expr[0])) {
            handleError(integer_expected(expr[0]));
            return false;
        }
        if (!is_const_integer(expr[1]) && !is_clock(expr[1])) {
            handleError(clock_expected(expr[1]));
            return false;
        }
        if (!isCompileTimeComputable(expr[2])) {
            handleError(must_be_computable_at_compile_time(expr[1]));
            return false;
        }
        if (!is_integral(expr[2])) {
            handleError(integer_expected(expr[2]));
            return false;
        }
        uint32_t nb = expr.get_size();
        if (expr.get_kind() == SIMULATEREACH) {
            bool ok = true;
            nb -= 2;
            if (!is_const_integer(expr[nb + 1])) {  // check number of accepting runs is an integer
                handleError(integer_expected(expr[nb + 1]));
                return false;
            }
            if (!is_integral(expr[nb]) && !is_constraint(expr[nb + 1])) {  // check reachability expression is a boolean
                handleError(boolean_expected(expr[nb]));
                ok = false;
            }
            if (expr[nb].changes_any_variable()) {  // check reachability expression is side effect free
                handleError(must_be_side_effect_free(expr[nb]));
                ok = false;
            }
            if (!ok)
                return false;
        }
        for (uint32_t i = 3; i < nb; ++i) {
            if (!is_integral(expr[i]) && !is_clock(expr[i]) && !is_double_value(expr[i]) &&
                !expr[i].get_type().is(Constants::DOUBLE_INV_GUARD) && !is_constraint(expr[i]) &&
                !expr[i].get_type().is_record() && !expr[i].get_type().is_array()) {
                handleError(integer_or_clock_expected(expr[i]));
                return false;
            }
            if (expr[i].changes_any_variable()) {
                handleError(must_be_side_effect_free(expr[i]));
                return false;
            }
        }
        type = Type::create_primitive(FORMULA);
        break;
    }

    case SUP_VAR:
    case INF_VAR:
    case BOUNDS_VAR:
        if (!is_integral(expr[0]) && !is_constraint(expr[0])) {
            handleError(boolean_expected(expr[0]));
            return false;
        }
        if (expr[1].get_kind() == LIST) {
            for (uint32_t i = 0; i < expr[1].get_size(); ++i) {
                if (is_integral(expr[1][i])) {
                    if (expr[1][i].changes_any_variable()) {
                        handleError(must_be_side_effect_free(expr[1][i]));
                        return false;
                    }
                } else if (!is_clock(expr[1][i])) {
                    handleError(clock_expected(expr[1][i]));
                    return false;
                }
            }
            type = Type::create_primitive(FORMULA);
        }
        break;

    case MITL_FORMULA:
    case MITL_CONJ:
    case MITL_DISJ:
    case MITL_NEXT:
    case MITL_UNTIL:
    case MITL_RELEASE:
    case MITL_ATOM:
        type = Type::create_primitive(FORMULA);
        // TODO
        break;
    case MIN_EXP:
    case MAX_EXP:
        // 0 = bound clock, 1 = boundvar, 2 = goal, 3 = price
        if (!is_const_integer(expr[0]) && !is_clock(expr[0])) {
            handleError(integer_or_clock_expected(expr[0]));
            ok = false;
        }
        if (!isCompileTimeComputable(expr[1])) {
            handleError(must_be_computable_at_compile_time(expr[1]));
            ok = false;
        }
        if (!ok)
            return false;

        if (is_formula(expr[2]))
            type = Type::create_primitive(FORMULA);

        for (uint32_t i = 3; i < expr.get_size(); ++i) {
            if (expr[i].changes_any_variable()) {
                handleError(must_be_side_effect_free(expr[i]));
                return false;
            }
        }
        break;
    case SMC_CONTROL:
        if (expr.get_size() == 3) {
            ok &= checkBoundTypeOrBoundedExpr(expr[0]);
            ok &= checkBound(expr[1]);
            if (!ok)
                return false;
            if (is_formula(expr[2])) {
                type = Type::create_primitive(FORMULA);
            }
        } else {
            handleError(bug_wrong_number_of_args(expr));
            return false;
        }
        break;

    case PROBA_MIN_BOX:
    case PROBA_MIN_DIAMOND:
        if (expr.get_size() == 5) {
            bool ok = true;
            ok &= checkNrOfRuns(expr[0]);
            if (ok && expr[0].get_value() > 0) {
                handleError(explicit_number_of_runs_is_not_supported_here(expr[0]));
                ok = false;
            }
            ok &= checkBoundTypeOrBoundedExpr(expr[1]);
            ok &= checkBound(expr[2]);
            ok &= checkPredicate(expr[3]);
            ok &= checkProbBound(expr[4]);
            if (!ok)
                return false;
            type = Type::create_primitive(FORMULA);
        } else {
            handleError(bug_wrong_number_of_args(expr));
            return false;
        }
        break;

    case PROBA_BOX:
    case PROBA_DIAMOND:
        if (expr.get_size() == 5) {
            bool ok = true;
            ok &= checkNrOfRuns(expr[0]);
            ok &= checkBoundTypeOrBoundedExpr(expr[1]);
            ok &= checkBound(expr[2]);
            ok &= checkPredicate(expr[3]);
            ok &= checkUntilCond(expr.get_kind(), expr[4]);
            if (!ok)
                return false;
            type = Type::create_primitive(FORMULA);
        } else {
            handleError(bug_wrong_number_of_args(expr));
            return false;
        }
        break;

    case PROBA_CMP:
        if (expr.get_size() == 8) {
            bool ok = true;
            // the first prob property:
            ok &= checkBoundTypeOrBoundedExpr(expr[0]);
            ok &= checkBound(expr[1]);
            ok &= checkPathQuant(expr[2]);
            ok &= checkPredicate(expr[3]);
            // the second prob property:
            ok &= checkBoundTypeOrBoundedExpr(expr[4]);
            ok &= checkBound(expr[5]);
            ok &= checkPathQuant(expr[6]);
            ok &= checkPredicate(expr[7]);
            if (!ok)
                return false;
            type = Type::create_primitive(FORMULA);
        } else {
            handleError(bug_wrong_number_of_args(expr));
            return false;
        }
        break;
    case PROBA_EXP:
        if (expr.get_size() == 5) {
            bool ok = true;
            ok &= checkNrOfRuns(expr[0]);
            ok &= checkBoundTypeOrBoundedExpr(expr[1]);
            ok &= checkBound(expr[2]);
            ok &= checkAggregationOp(expr[3]);
            ok &= checkMonitoredExpr(expr[4]);
            if (!ok)
                return false;
            type = Type::create_primitive(FORMULA);
        } else {
            handleError(bug_wrong_number_of_args(expr));
            return false;
        }
        break;
    default: return true;
    }

    if (type.unknown()) {
        handleError(unknown_type(expr));
        return false;
    } else {
        expr.set_type(type);
        return true;
    }
}

/**
 * Returns true if expression evaluates to a modifiable l-value.
 */
bool TypeChecker::isModifiableLValue(const Expression& expr) const
{
    switch (expr.get_kind()) {
    case IDENTIFIER: return expr.get_type().is_mutable();

    case DOT:
        /* Processes can only be used in properties, which must be
         * side-effect anyway. Therefore, returning false below is
         * acceptable for now (REVISIT).
         */
        if (expr[0].get_type().is_process()) {
            return false;
        }
        // REVISIT: Not correct if records contain constant fields.
        return isModifiableLValue(expr[0]);

    case ARRAY: return isModifiableLValue(expr[0]);

    case PRE_INCREMENT:
    case PRE_DECREMENT:
    case ASSIGN:
    case ASS_PLUS:
    case ASS_MINUS:
    case ASS_DIV:
    case ASS_MOD:
    case ASS_MULT:
    case ASS_AND:
    case ASS_OR:
    case ASS_XOR:
    case ASS_LSHIFT:
    case ASS_RSHIFT: return true;

    case INLINE_IF:
        return isModifiableLValue(expr[1]) && isModifiableLValue(expr[2]) &&
               expr[1].get_type().is_equivalent(expr[2].get_type());

    case COMMA: return isModifiableLValue(expr[1]);

    case FUN_CALL:
    case FUN_CALL_EXT:
        // Functions cannot return references (yet!)

    default: return false;
    }
}

/**
 * Returns true iff \a expr evaluates to an lvalue.
 */
bool TypeChecker::isLValue(const Expression& expr) const
{
    switch (expr.get_kind()) {
    case IDENTIFIER:
    case PRE_INCREMENT:
    case PRE_DECREMENT:
    case ASSIGN:
    case ASS_PLUS:
    case ASS_MINUS:
    case ASS_DIV:
    case ASS_MOD:
    case ASS_MULT:
    case ASS_AND:
    case ASS_OR:
    case ASS_XOR:
    case ASS_LSHIFT:
    case ASS_RSHIFT: return true;

    case DOT:
    case ARRAY: return isLValue(expr[0]);

    case INLINE_IF:
        return isLValue(expr[1]) && isLValue(expr[2]) && expr[1].get_type().is_equivalent(expr[2].get_type());

    case COMMA: return isLValue(expr[1]);

    case FUN_CALL:
    case FUN_CALL_EXT:
        // Functions cannot return references (yet!)

    default: return false;
    }
}

/** Returns true if expression is a reference to a unique variable.
    This is similar to expr being an l-value, but in addition we
    require that the reference does not depend on any non-computable
    expressions. Thus i[v] is a l-value, but if v is a non-constant
    variable, then it does not result in a unique reference.
*/
bool TypeChecker::isUniqueReference(const Expression& expr) const
{
    switch (expr.get_kind()) {
    case IDENTIFIER: return true;

    case DOT: return isUniqueReference(expr[0]);

    case ARRAY: return isUniqueReference(expr[0]) && isCompileTimeComputable(expr[1]);

    case PRE_INCREMENT:
    case PRE_DECREMENT:
    case ASSIGN:
    case ASS_PLUS:
    case ASS_MINUS:
    case ASS_DIV:
    case ASS_MOD:
    case ASS_MULT:
    case ASS_AND:
    case ASS_OR:
    case ASS_XOR:
    case ASS_LSHIFT:
    case ASS_RSHIFT: return isUniqueReference(expr[0]);

    case INLINE_IF: return false;

    case COMMA: return isUniqueReference(expr[1]);

    case FUN_CALL:
    case FUN_CALL_EXT:
        // Functions cannot return references (yet!)

    default: return false;
    }
}

bool parse_XTA(FILE* file, Document& doc, bool newxta)
{
    auto builder = DocumentBuilder{doc};
    parse_XTA(file, builder, newxta);
    static_analysis(doc);
    return !doc.has_errors();
}

bool parse_XTA(const char* buffer, Document& doc, bool newxta)
{
    auto builder = DocumentBuilder{doc};
    parse_XTA(buffer, builder, newxta);
    static_analysis(doc);
    return !doc.has_errors();
}

int32_t parse_XML_buffer(const char* buffer, Document& doc, bool newxta,
                         const std::vector<std::filesystem::path>& paths)
{
    auto builder = DocumentBuilder{doc, paths};
    if (const auto err = parse_XML_buffer(buffer, builder, newxta); err != 0)
        return err;
    static_analysis(doc);
    return 0;
}

int32_t parse_XML_file(const std::filesystem::path& file, Document& doc, bool newxta,
                       const std::vector<std::filesystem::path>& paths)
{
    auto builder = DocumentBuilder{doc, paths};
    if (const auto err = parse_XML_file(file, builder, newxta); err != 0)
        return err;
    static_analysis(doc);
    return 0;
}

int32_t parse_XML_fd(int fd, Document& doc, bool newxta, const std::vector<std::filesystem::path>& paths)
{
    auto builder = DocumentBuilder{doc, paths};
    if (const auto err = parse_XML_fd(fd, builder, newxta); err != 0)
        return err;
    static_analysis(doc);
    return 0;
}

Expression parseExpression(const char* str, Document& doc, bool newxtr)
{
    auto builder = ExpressionBuilder{doc};
    parse_XTA(str, builder, newxtr, S_EXPRESSION, "");
    Expression expr = builder.get_expressions()[0];
    if (!doc.has_errors()) {
        auto checker = TypeChecker{doc};
        checker.checkExpression(expr);
    }
    return expr;
}

bool TypeChecker::visit_template_before(Template& t)
{
    assert(!temp);
    temp = &t;
    return true;
}

void TypeChecker::visit_template_after(Template& t)
{
    assert(&t == temp);
    temp = nullptr;
}

bool TypeChecker::checkSpawnParameterCompatible(const Type& param, const Expression& arg)
{
    return checkParameterCompatible(param, arg);
}

bool TypeChecker::checkDynamicExpressions(Statement& stat)
{
    auto expr_list = collect_dynamic_expressions(stat);
    bool ok = true;
    for (const auto& expr : expr_list) {
        ok = false;
        handleError(dynamic_constructs_supported_only_on_edges(expr));
    }
    return ok;
}

bool TypeChecker::checkNrOfRuns(const Expression& runs)
{
    if (!isCompileTimeComputable(runs)) {
        handleError(must_be_computable_at_compile_time(runs));
        return false;
    }
    if (!is_const_integer(runs)) {
        handleError(integer_expected(runs));
        return false;
    }
    return true;
}

bool TypeChecker::checkBoundTypeOrBoundedExpr(const Expression& boundTypeOrExpr)
{
    if (!is_const_integer(boundTypeOrExpr) && !is_clock(boundTypeOrExpr)) {
        handleError(clock_expected(boundTypeOrExpr));
        return false;
    }
    return true;
}

bool TypeChecker::checkBound(const Expression& bound)
{
    if (!isCompileTimeComputable(bound)) {
        handleError(must_be_computable_at_compile_time(bound));
        return false;
    }
    if (!is_integral(bound)) {
        handleError(integer_expected(bound));
        return false;
    }
    return true;
}

bool TypeChecker::checkPredicate(const Expression& predicate)
{
    if (!is_integral(predicate) && !is_constraint(predicate)) {  // check reachability expression is a boolean
        handleError(boolean_expected(predicate));
        return false;
    }
    if (predicate.changes_any_variable()) {  // check reachability expression is side effect free
        handleError(must_be_side_effect_free(predicate));
        return false;
    }
    return true;
}

bool TypeChecker::checkProbBound(const Expression& probBound)
{
    if (!is_const_double(probBound)) {
        handleError(floating_point_expected(probBound));
        return false;
    }
    return true;
}

bool TypeChecker::checkUntilCond(Kind kind, const Expression& untilCond)
{
    bool ok = true;
    if (kind == PROBA_DIAMOND && !is_integral(untilCond) && !is_constraint(untilCond)) {
        handleError(boolean_expected(untilCond));
        ok = false;
    }
    if (kind == PROBA_BOX && untilCond.get_kind() == Constants::BOOL && untilCond.get_value() != 0) {
        handleError(must_be_false(untilCond));  // TODO - error message
        ok = false;
    }
    return ok;
}

bool TypeChecker::checkMonitoredExpr(const Expression& expr)
{
    if (!is_integral(expr) && !is_clock(expr) && !is_double_value(expr) &&
        !expr.get_type().is(Constants::DOUBLE_INV_GUARD) && !is_constraint(expr)) {
        handleError(integer_or_clock_expected(expr));
        return false;
    }
    if (expr.changes_any_variable()) {
        handleError(must_be_side_effect_free(expr));
        return false;
    }
    return true;
}

bool TypeChecker::checkPathQuant(const Expression& expr)
{
    if (!is_const_integer(expr)) {
        handleError(bug_bad_path_quantifier(expr));
        return false;
    }
    return true;
}

bool TypeChecker::checkAggregationOp(const Expression& expr)
{
    if (!is_const_integer(expr)) {
        handleError(bug_bad_aggregation_expression(expr));
        return false;
    }
    if (expr.get_value() > 1)  // 0="min", 1="max"
    {
        handleError(bug_bad_aggregation_value(expr));
        return false;
    }
    return true;
}
