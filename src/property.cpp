// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2011-2023 Aalborg University.
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

#include "utap/property.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <utility>

namespace UTAP {

using namespace UTAP::Constants;

static void parse_expect(std::string_view expect, PropInfo& info)
{
    using std::cerr;
    using std::endl;
    if (expect.empty())
        return;
    auto exp_u = std::string{expect};
    std::transform(begin(exp_u), end(exp_u), begin(exp_u), ::toupper);
    auto is = std::istringstream(exp_u);
    std::string token;
    while (getline(is, token, ',')) {
        if (token == "T" || token == "SAT" || token == "SATISFIED" || token == "TRUE") {
            info.set_expect(status_t::DONE_TRUE);
        } else if (token == "F" || token == "UNSAT" || token == "UNSATISFIED" || token == "FALSE") {
            info.set_expect(status_t::DONE_FALSE);
        } else if (token == "MT" || token == "MSAT" || token == "MAYBE_SATISFIED" || token == "MAYBE_TRUE") {
            info.set_expect(status_t::DONE_MAYBE_TRUE);
        } else if (token == "MF" || token == "MUNSAT" || token == "MAYBE_UNSATISFIED" || token == "MAYBE_FALSE") {
            info.set_expect(status_t::DONE_MAYBE_FALSE);
        } else if (token == "E" || token == "ERR" || token == "ERROR") {
            info.set_expect(status_t::DONE_ERROR);
        } else {
            auto is = std::istringstream(token);
            double number;
            std::string unit;
            if (is >> number) {
                if (is >> unit) {
                    if (unit == "B")
                        info.set_expect_mem(static_cast<uint64_t>(number / 1024));
                    else if (unit == "KB")
                        info.set_expect_mem(static_cast<uint64_t>(number));
                    else if (unit == "MB")
                        info.set_expect_mem(static_cast<uint64_t>(number * 1024));
                    else if (unit == "GB")
                        info.set_expect_mem(static_cast<uint64_t>(number * 1024 * 1024));
                    else if (unit == "TB")
                        info.set_expect_mem(static_cast<uint64_t>(number * 1024 * 1024 * 1024));
                    else if (unit == "MS")
                        info.set_expect_time(static_cast<uint64_t>(number));
                    else if (unit == "S")
                        info.set_expect_time(static_cast<uint64_t>(number * 1000));
                    else if (unit == "M")
                        info.set_expect_time(static_cast<uint64_t>(number * 1000 * 60));
                    else if (unit == "H")
                        info.set_expect_time(static_cast<uint64_t>(number * 1000 * 60 * 60));
                    else if (unit == "D")
                        info.set_expect_time(static_cast<uint64_t>(number * 1000 * 60 * 60 * 24));
                    else
                        cerr << "Could not parse EXPECT token: " << token << endl;
                } else {
                    info.set_expect(number);
                }
            } else
                cerr << "Could not parse EXPECT token: " << token << endl;
        }
    }
}

void PropertyBuilder::property()
{
    // Construct expression
    Expression expr = fragments.pop();

    /* Type check expression. */
    tc.visitProperty(expr);
    if (document.has_errors())
        return;

    if (document.has_priority_declaration() || document.has_clock_guard_recv_broadcast()) {
        if (  // expr.get_kind() == AF ||
              // expr.get_kind() == EG ||
              // expr.get_kind() == LEADS_TO ||
            expr.get_kind() == SCENARIO || expr.get_kind() == SCENARIO2) {
            throw TypeException("$Cannot_handle_this_formula_for_models_with_priorities_or_guarded_broadcast_"
                                      "receivers");
            }
        // Undid Marius' change.
        // The error says clearly for models with priorities or guarded broadcast.
        // The reason is theoretical. There is no proof of correctness for either of these
        // cases. In fact they are very similar because they are both based on partitioning
        // the states with subtractions. This was added in rev. 4528.
        if (/*document->has_priority_declaration() &&*/ expr.contains_deadlock())
            throw TypeException(
                "$Cannot_handle_deadlock_predicate_for_models_with_priorities_or_guarded_broadcast_"
                "receivers");
    }

    if (expr.get_kind() != EF && expr.get_kind() != AG && expr.contains_deadlock()) {
        throw TypeException("$Cannot_handle_this_deadlock_predicate");
    }

    if (document.has_dynamic_templates() && !is_SMC(&expr))
        throw TypeException("Dynamic templates are only supported for SMC queries");

    /* Compile expression. */
    properties.emplace_back(document.find_position(position.start).line, properties.size(), expr);
    parse_expect(lastExpect, properties.back());

    type_property(expr);
}

static bool symbolicProperty(const Expression& expr)
{
    switch (expr.get_kind()) {
    case EF:
    case EG:
    case AF:
    case AG:
    case LEADS_TO:
    case A_UNTIL:
    case A_WEAK_UNTIL:
    case A_BUCHI:
    case BOX:
    case DIAMOND:
    case CONTROL:
    case PO_CONTROL:
    case EF_CONTROL:
    case CONTROL_TOPT:
    case CONTROL_TOPT_DEF1:
    case CONTROL_TOPT_DEF2:
    case SUP_VAR:
    case INF_VAR:
    case BOUNDS_VAR:
    case SCENARIO:
    case SCENARIO2: return true;
    default: return false;
    }
}

// Should not lint, because TigaPropertyBuilder::typeProperty does
// change the argument.
// Ideally TigaPropertyBuilder::typeProperty should just be fixed as well
void PropertyBuilder::type_property(Expression expr)  // NOLINT
{
    bool prob = false;

    switch (expr.get_kind()) {
    case EF: properties.back().type = quant_t::EE; break;
    case EG: properties.back().type = quant_t::EG; break;
    case AF: properties.back().type = quant_t::AE; break;
    case AG: properties.back().type = quant_t::AG; break;
    case LEADS_TO: properties.back().type = quant_t::leads_to; break;
    case SCENARIO:
    case SCENARIO2:
        properties.back().type = quant_t::scenario;  // std::cout << "pScenario" << std::endl;
        break;
    case SUP_VAR: properties.back().type = quant_t::supremum; break;
    case INF_VAR: properties.back().type = quant_t::infimum; break;
    case BOUNDS_VAR: properties.back().type = quant_t::bounds; break;
    case PROBA_MIN_BOX:
        properties.back().type = quant_t::probaMinBox;
        prob = true;
        break;
    case PROBA_MIN_DIAMOND:
        properties.back().type = quant_t::probaMinDiamond;
        prob = true;
        break;
    case PROBA_BOX:
        properties.back().type = quant_t::probaBox;
        prob = true;
        break;
    case PROBA_DIAMOND:
        properties.back().type = quant_t::probaDiamond;
        prob = true;
        break;
    case PROBA_EXP:
        properties.back().type = quant_t::probaExpected;
        prob = true;
        break;
    case PROBA_CMP:
        properties.back().type = quant_t::probaCompare;
        prob = true;
        break;
    case SIMULATE:
        properties.back().type = quant_t::probaSimulate;
        prob = true;
        break;
    case SIMULATEREACH:
        properties.back().type = quant_t::probaSimulateReach;
        prob = true;
        break;
    case MITL_FORMULA: properties.back().type = quant_t::Mitl; break;
    default: throw TypeException("$Invalid_property_type"); prob = true;
    }

    if (prob) {
        if (document.has_priority_declaration())
            throw TypeException("Priorities are not supported");
        if (!document.all_broadcast())
            throw TypeException("All channels must be broadcast");
    }
    if (document.get_sync_used() == 2 && document.has_priority_declaration())
        throw TypeException("CSP synchronization is not implemented with priorities.");
    if (symbolicProperty(expr) && (expr.uses_hybrid() || expr.uses_fp()))
        throw TypeException("Symbolic verification and synthesis exclude usage of doubles and hybrid clocks in "
                                  "properties.");
}

void PropertyBuilder::scenario(std::string_view name)
{
    Symbol symbol, i_symbol;
    if (!resolve(name, symbol))
        throw std::runtime_error("$No_such_scenario: " + std::string{name});
    Type type = symbol.get_type();
    if (type.get_kind() != LSC_INSTANCE)
        throw std::runtime_error("$Not_a_LSC_template: " + symbol.get_name());
}

void PropertyBuilder::handle_expect(std::string_view text)
{
    if (not text.empty() && not properties.empty())
        parse_expect(text, properties.back());
}

bool PropertyBuilder::allow_process_references() { return true; }

void PropertyBuilder::parse(const char* buf) { parse_property(buf, *this); }

void PropertyBuilder::parse(FILE* file)
{
    clear();
    parse_property(file, *this);
}

void PropertyBuilder::parse(const char* buf, const std::string& xpath, const Options& options)
{
    size_t num_props = properties.size();
    parse_property(buf, *this, xpath);
    // if buffer contained no property, we must not set options at end of list.
    // in particular, if the first query is empty, this assignment would segfault.
    if (properties.size() > num_props)
        properties.back().options = options;
}

Variable* PropertyBuilder::add_variable(Type type, std::string_view name, Expression init, position_t pos)
{
    throw NotSupportedException("addVariable is not supported");
}

bool PropertyBuilder::add_function(Type type, std::string_view name, position_t pos)
{
    throw NotSupportedException("addFunction is not supported");
}

bool PropertyBuilder::is_SMC(const Expression* expr) const
{
    if (expr == nullptr)
        expr = &(fragments[0]);
    Kind k = expr->get_kind();
    return (k == PMAX || k == PROBA_MIN_BOX || k == PROBA_MIN_DIAMOND || k == PROBA_BOX || k == PROBA_DIAMOND ||
            k == PROBA_CMP || k == PROBA_EXP || k == SIMULATE || k == SIMULATEREACH || k == MITL_FORMULA ||
            k == MIN_EXP || k == MAX_EXP);
}

void TigaPropertyBuilder::type_property(Expression expr)
{
    bool potigaProp = false;
    bool titiga = false;
    bool prob = false;

    properties.back().subjections = subjections;
    properties.back().imitation = _imitation;
    subjections.clear();
    _imitation = nullptr;

    switch (expr.get_kind()) {
    case LOAD_STRAT:
        properties.back().result_type = NonZoneStrategy;
        properties.back().type = quant_t::strategy_load;
        break;
    case SAVE_STRAT: properties.back().type = quant_t::strategy_save; break;
    case MIN_EXP:
        properties.back().result_type = NonZoneStrategy;
        properties.back().type = quant_t::control_MinExp;
        prob = true;
        break;
    case MAX_EXP:
        properties.back().result_type = NonZoneStrategy;
        properties.back().type = quant_t::control_MaxExp;
        prob = true;
        break;
    case SMC_CONTROL:
        assert(false);  // FIXME: why allow this in release?
        prob = true;
        // expr[0] = bound type|clock
        // expr[1] = bound
        // expr[2] = SubProperty
        properties.back().result_type = NonZoneStrategy;
        switch (expr[2].get_kind()) {
        case A_UNTIL: properties.back().type = quant_t::control_SMC_AUntil; break;
        case AF: properties.back().type = quant_t::control_SMC_AF; break;
        default: throw TypeException("$Invalid_control_synthesis_property_type");
        }
        break;

    case CONTROL:
        expr = expr[0];
        properties.back().result_type = ZoneStrategy;
        properties.back().intermediate = expr;
        switch (expr.get_kind()) {
        case AF: properties.back().type = quant_t::control_AF; break;
        case A_UNTIL: properties.back().type = quant_t::control_AUntil; break;
        case AG:
            if (expr[0].get_kind() == AF) {
                expr = expr[0];
                properties.back().intermediate = expr;
                properties.back().type = quant_t::control_AB;
            } else if (expr[0].get_kind() == AND && expr[0][1].get_kind() == AF) {
                expr = Expression::create_binary(A_BUCHI, expr[0][0], expr[0][1][0], expr[0].get_position(),
                                                 expr[0].get_type());
                properties.back().intermediate = expr;
                properties.back().type = quant_t::control_ABuchi;
            } else {
                properties.back().type = quant_t::control_AG;
            }
            break;
        case A_WEAK_UNTIL: properties.back().type = quant_t::control_AWeakUntil; break;
        default: throw TypeException("$Invalid_control_synthesis_property_type");
        }
        break;

    case EF_CONTROL:
        expr = expr[0];
        properties.back().result_type = ZoneStrategy;
        properties.back().intermediate = expr;
        switch (expr.get_kind()) {
        case AF: properties.back().type = quant_t::EF_control_AF; break;
        case A_UNTIL: properties.back().type = quant_t::EF_control_AUntil; break;
        case AG: properties.back().type = quant_t::EF_control_AG; break;
        case A_WEAK_UNTIL: properties.back().type = quant_t::EF_control_AWeakUntil; break;
        default: throw TypeException("$Invalid_control_synthesis_property_type");
        }
        break;

    case PO_CONTROL:
        potigaProp = true;
        properties.back().intermediate = expr;
        switch (expr[1].get_kind()) {
        case AF: properties.back().type = quant_t::PO_control_AF; break;
        case AG: properties.back().type = quant_t::PO_control_AG; break;
        case A_UNTIL: properties.back().type = quant_t::PO_control_AUntil; break;
        case A_WEAK_UNTIL: properties.back().type = quant_t::PO_control_AWeakUntil; break;
        default: throw TypeException("$Invalid_control_synthesis_property_type");
        }
        break;

    case CONTROL_TOPT:
        properties.back().result_type = ZoneStrategy;
        properties.back().intermediate = expr;
        switch (expr[2].get_kind()) {
        case AF: properties.back().type = quant_t::control_opt_AF; break;
        case A_UNTIL: properties.back().type = quant_t::control_opt_AUntil; break;
        default: throw TypeException("$Invalid_type_of_time_optimal_control_synthesis_property");
        }
        break;

    case CONTROL_TOPT_DEF1:
        properties.back().result_type = ZoneStrategy;
        properties.back().intermediate = expr;
        switch (expr[1].get_kind()) {
        case AF: properties.back().type = quant_t::control_opt_Def1_AF; break;
        case A_UNTIL: properties.back().type = quant_t::control_opt_Def1_AUntil; break;
        default: throw TypeException("$Invalid_type_of_time_optimal_control_synthesis_property");
        }
        break;

    case CONTROL_TOPT_DEF2:
        expr = expr[0];
        properties.back().result_type = ZoneStrategy;
        properties.back().intermediate = expr;
        switch (expr.get_kind()) {
        case AF: properties.back().type = quant_t::control_opt_Def2_AF; break;
        case A_UNTIL: properties.back().type = quant_t::control_opt_Def2_AUntil; break;
        default: throw TypeException("$Invalid_type_of_time_optimal_control_synthesis_property");
        }
        break;

    default: return PropertyBuilder::type_property(expr);
    }

    if (prob && !document.all_broadcast())
        throw TypeException("All channels must be broadcast");

    if (potigaProp && document.has_strict_lower_bound_on_controllable_edges())
        throw TypeException("$(PO)TIGA_properties_cannot_be_checked_for_systems_with_strict_lower_bounds_in_"
                                  "guards");
    if (titiga && document.has_priority_declaration())  // FIXME: always false
        throw TypeException("$Priorities_are_not_yet_supported_in_TIGA");
    if (!prob) {  // FIXME: always true
        if (document.has_strict_invariants())
            throw TypeException("$TIGA_properties_cannot_be_checked_for_systems_with_strict_invariants");
        // Stop-watches are now checked on-the-fly for SMC compatibility.
        // if (document->has_stop_watch(())
        //    throw TypeException("$Stop_watches_are_not_yet_supported_in_TIGA");
    }
}

void TigaPropertyBuilder::strategy_declaration(std::string_view id)
{
    if (auto it = declarations.find(id); it != declarations.end()) {
        declarations.erase(it);
        handle_warning(duplicate_definition_error(id));
    }
    declarations.emplace(std::string(id), &properties.back());
    if (!properties.empty())  // this happens when the model and the query file do not correspond.
        properties.back().declaration = id;
}

void TigaPropertyBuilder::subjection(std::string_view id)
{
    if (auto it = declarations.find(id); it != declarations.end())
        subjections.push_back(it->second);
    else
        handle_error(strategy_not_declared_error(id));
}

void TigaPropertyBuilder::imitation(std::string_view id)
{
    if (auto it = declarations.find(id); it != declarations.end())
        _imitation = it->second;
    else
        handle_error(strategy_not_declared_error(id));
}

void TigaPropertyBuilder::expr_optimize(int, int, int, int)
{
    // nothing for now
}
} // namespace UTAP