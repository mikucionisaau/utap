// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020-2024 Aalborg University.
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

#include "utap/FeatureChecker.hpp"

#include "utap/common.hpp"
#include "utap/document.hpp"

#include <cassert>

namespace UTAP {

FeatureChecker::FeatureChecker(Document& document)
{
    document.accept(*this);
    visit_frame(document.get_globals().frame);
    if (document.has_dynamic_templates())
        supported_methods.symbolic = false;
    if (document.has_priority_declaration()) {
        supported_methods.stochastic = false;
        supported_methods.concrete = false;
    }
}

bool FeatureChecker::visit_template_before(Template& templ)
{
    // Only check features if template is actually used in the system
    return templ.is_instantiated;
}

void FeatureChecker::visit_variable(Variable& var)
{
    if (var.uid.get_type().is_clock() && !var.init.empty() && var.init.uses_fp())
        supported_methods.symbolic = false;
}

void FeatureChecker::visit_edge(Edge& edge)
{
    visit_assignment(edge.assign);
    visit_guard(edge.guard);
}

void FeatureChecker::visit_guard(Expression& guard)
{
    switch (guard.get_kind()) {
    case Constants::LT:
    case Constants::LE:
    case Constants::EQ:
        for (auto i = 0u; i < guard.get_size(); ++i) {
            if (guard.get(i).uses_fp())
                supported_methods.symbolic = false;
        }
        [[fallthrough]];
    default: break;
    }
}

void FeatureChecker::visit_assignment(Expression& ass)
{
    switch (ass.get_kind()) {
    case Constants::ASSIGN:
        if (ass.uses_fp() && !ass.uses_hybrid())
            supported_methods.symbolic = false;
        break;
    case Constants::COMMA:
        for (auto i = 0u; i < ass.get_size(); ++i)
            visit_assignment(ass.get(i));
        break;
    default: break;
    }
}

void FeatureChecker::visit_location(Location& location)
{
    const auto& invariant = location.invariant;
    if (invariant.empty())
        return;
    if (is_rate_disallowed_in_symbolic(invariant))
        supported_methods.symbolic = false;
}

/**
 * Checks whether an expression sets the rate of a clock
 * to something that is different from one or zero
 */
bool FeatureChecker::is_rate_disallowed_in_symbolic(const Expression& e)
{
    if (e.get_kind() == Constants::EQ) {
        assert(e.get_size() >= 2);

        Expression rate;
        Expression clock;

        if (e.get(0).get_kind() == Constants::RATE) {
            clock = e.get(0);
            rate = e.get(1);
        } else if (e.get(1).get_kind() == Constants::RATE) {
            clock = e.get(1);
            rate = e.get(0);
        } else {
            return false;
        }

        // rates over hybrid clocks are allowed, because they are ignored/abstracted in symbolic analysis
        if (clock.get(0).get_symbol().get_type().is(Constants::HYBRID))
            return false;

        if (rate.get_kind() != Constants::CONSTANT)
            return false;
        if (rate.get_value() != 0 && rate.get_value() != 1)
            return true;  // NOLINT(readability-simplify-boolean-expr)

        return false;
    }

    if (e.get_kind() == Constants::AND) {
        for (uint32_t i = 0; i < e.get_size(); ++i) {
            if (is_rate_disallowed_in_symbolic(e.get(i)))
                return true;
        }
        return false;
    }
    return false;
}

void FeatureChecker::visit_frame(const Frame& frame)
{
    for (uint32_t i = 0; i < frame.get_size(); ++i) {
        Type t = frame.get_symbol(i).get_type();
        if (t.is_channel() && !t.is(Constants::BROADCAST))
            supported_methods.stochastic = false;
    }
}

} // namespace UTAP