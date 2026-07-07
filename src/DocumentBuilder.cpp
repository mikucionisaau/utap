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

#include "utap/DocumentBuilder.hpp"

#include "print.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>
#include <cassert>

using namespace UTAP;
using namespace Constants;
using namespace std::string_view_literals;

DocumentBuilder::DocumentBuilder(Document& doc, std::vector<std::filesystem::path> paths):
    StatementBuilder{doc, std::move(paths)}
{
    declarations.push(&doc.get_globals());
}

/************************************************************
 * Variable and function declarations
 */
Variable* DocumentBuilder::add_variable(Type type, std::string_view name, Expression init, position_t pos)
{
    if (currentFun != nullptr) {
        return document.add_variable_to_function(currentFun, frames.top(), type, name, init, pos);
    }
    return document.add_variable(get_current_declaration_block(), frames.top(), type, name, init, pos);
}

bool DocumentBuilder::add_function(Type type, std::string_view name, position_t pos)
{
    return get_current_declaration_block()->add_function(type, name, pos, currentFun);
}

Declarations* DocumentBuilder::get_current_declaration_block() { return declarations.top(); }

void DocumentBuilder::system_decl_begin()
{
    declarations.push(&document.get_system_declarations());
    push_frame(document.get_system_declarations().frame);
}

void DocumentBuilder::system_decl_end()
{
    declarations.pop();
    pop_frame();
}

void DocumentBuilder::add_select_symbol_to_frame(std::string_view id, Frame& frame, position_t pos)
{
    auto type = typeFragments.pop();
    if (!type.is(CONSTANT)) {
        type = type.create_prefix(CONSTANT);
    }
    if (!type.is_scalar() && !type.is_integer()) {
        handle_error(TypeException{"$Scalar_set_or_integer_expected"});
    } else if (!type.is(RANGE)) {
        handle_error(TypeException{"$Range_expected"});
    } else {
        if (auto uid = Symbol{}; resolve(id, uid))
            handle_warning(shadows_a_variable_warning(id));
        frame.add_symbol(id, type, pos);
    }
}

/************************************************************
 * Gantt chart
 */

void DocumentBuilder::gantt_decl_begin(std::string_view name)
{
    currentGantt = std::make_unique<GanttEntry>(name);
    push_frame(frames.top().make_sub());
}

void DocumentBuilder::gantt_decl_select(std::string_view id) { add_select_symbol_to_frame(id, frames.top(), position); }

void DocumentBuilder::gantt_decl_end()
{
    currentGantt->parameters = pop_top(frames);
    document.add_gantt(get_current_declaration_block(), std::move(*currentGantt));
    currentGantt.reset();
}

void DocumentBuilder::gantt_entry_begin() { push_frame(frames.top().make_sub()); }

void DocumentBuilder::gantt_entry_select(std::string_view id) { add_select_symbol_to_frame(id, frames.top(), position); }

void DocumentBuilder::gantt_entry_end()
{
    auto frame = pop_top(frames);
    auto mapping = fragments.pop();
    auto predicate = fragments.pop();
    currentGantt->mapping.emplace_back(std::move(frame), std::move(predicate), std::move(mapping));
}

/************************************************************
 * Guarded progress measure
 */
void DocumentBuilder::decl_progress(bool hasGuard)
{
    auto measure = fragments.pop();
    auto guard = Expression{};
    if (hasGuard)
        guard = fragments.pop();
    document.add_progress_measure(get_current_declaration_block(), guard, measure);
}

/********************************************************************
 * Process declarations
 */
void DocumentBuilder::proc_begin(std::string_view name, const bool isTA, std::string_view type, std::string_view mode)
{
    currentTemplate = document.find_dynamic_template(name);
    if (currentTemplate != nullptr) {
        // check if parameters match
        if (currentTemplate->parameters.get_size() != params.get_size()) {
            handle_error(TypeException{"Inconsistent parameters"});
        } else {
            for (uint32_t i = 0; i < params.get_size(); i++) {
                if (params[i].get_name() != currentTemplate->parameters[i].get_name() ||
                    params[i].get_type().get_kind() != currentTemplate->parameters[i].get_type().get_kind())
                    handle_error(TypeException{"Inconsistent parameters"});
            }
        }
        currentTemplate->is_defined = true;
    } else {
        if (frames.top().contains(name))
            handle_error(duplicate_definition_error(name));
        // search if the given name is a name of LSC
        auto lsc_name_it = find(lscTemplateNames.begin(), lscTemplateNames.end(), std::string(name));
        if (isTA && (lsc_name_it == lscTemplateNames.end())) {  // TA
            currentTemplate = &document.add_template(name, params, position);
        } else {  // LSC
            currentTemplate = &document.add_template(name, params, position, false, type, mode);
        }
    }

    declarations.push(currentTemplate);
    push_frame(currentTemplate->frame);
    params = Frame::make();
}

void DocumentBuilder::proc_end()  // 1 ProcBody
{
    declarations.pop();
    currentTemplate = nullptr;
    frames.pop();
}

/**
 * Add a state to the current template. An invariant expression is
 * expected on and popped from the expression stack if \a hasInvariant
 * is true.
 */
void DocumentBuilder::proc_location(std::string_view name, bool hasInvariant, bool hasER)  // 1 expr
{
    Expression e;
    Expression f;
    if (hasER)
        f = fragments.pop();
    if (hasInvariant)
        e = fragments.pop();
    currentTemplate->add_location(name, std::move(e), std::move(f), position);
}

void DocumentBuilder::proc_location_commit(std::string_view name)
{
    if (auto uid = Symbol{}; !resolve(name, uid) || !uid.get_type().is_location()) {
        handle_error(TypeException{"$Location_expected"});
    } else if (uid.get_type().is(URGENT)) {
        handle_error(TypeException{"$States_cannot_be_committed_and_urgent_at_the_same_time"});
    } else {
        uid.set_type(uid.get_type().create_prefix(COMMITTED, position));
    }
}

void DocumentBuilder::proc_location_urgent(std::string_view name)
{
    if (auto uid = Symbol{}; !resolve(name, uid) || !uid.get_type().is_location()) {
        handle_error(TypeException{"$Location_expected"});
    } else if (uid.get_type().is(COMMITTED)) {
        handle_error(TypeException{"$States_cannot_be_committed_and_urgent_at_the_same_time"});
    } else {
        uid.set_type(uid.get_type().create_prefix(URGENT, position));
    }
}

void DocumentBuilder::proc_branchpoint(std::string_view name) { currentTemplate->add_branchpoint(name, position); }

void DocumentBuilder::proc_location_init(std::string_view name)
{
    if (auto uid = Symbol{}; !resolve(name, uid) || !uid.get_type().is_location()) {
        handle_error(TypeException{"$Location_expected"});
    } else {
        currentTemplate->init = uid;
    }
}

void DocumentBuilder::proc_edge_begin(std::string_view from, std::string_view to, const bool control,
                                      std::string_view actname)
{
    if (auto fid = Symbol{};
        !resolve(from, fid) || (!fid.get_type().is_location() && !fid.get_type().is_branchpoint())) {
        handle_error(TypeException{"$No_such_location_or_branchpoint_(source)"});
        push_frame(frames.top().make_sub());  // dummy frame for upcoming popFrame
    } else if (auto tid = Symbol{};
               !resolve(to, tid) || (!tid.get_type().is_location() && !tid.get_type().is_branchpoint())) {
        handle_error(TypeException{"$No_such_location_or_branchpoint_(destination)"});
        push_frame(frames.top().make_sub());  // dummy frame for upcoming popFrame
    } else {
        currentEdge = &currentTemplate->add_edge(fid, tid, control, actname);
        currentEdge->guard = make_constant(1);
        currentEdge->assign = make_constant(1);
        // default "probability" weight is 1.
        currentEdge->prob = make_constant(1);
        push_frame(currentEdge->select = frames.top().make_sub());
    }
}

void DocumentBuilder::proc_edge_end(std::string_view from, std::string_view to) { frames.pop(); }

void DocumentBuilder::proc_select(std::string_view id) { add_select_symbol_to_frame(id, currentEdge->select, position); }

void DocumentBuilder::proc_guard()
{
    if (currentEdge == nullptr) {
        handle_error(TypeException("Must be declared inside of an edge"));
        return;
    }
    currentEdge->guard = fragments.pop();
}

void DocumentBuilder::proc_sync(Synchronisation type)
{
    if (currentEdge == nullptr) {
        handle_error(TypeException("Must be declared inside of an edge"));
        return;
    }
    currentEdge->sync = Expression::create_sync(fragments.pop(), type, position);
}

void DocumentBuilder::proc_update()
{
    if (currentEdge == nullptr) {
        handle_error(TypeException("Must be declared inside of an edge"));
        return;
    }
    currentEdge->assign = fragments.pop();
}

void DocumentBuilder::proc_prob()
{
    if (currentEdge == nullptr) {
        handle_error(TypeException("Must be declared inside of an edge"));
        return;
    }
    currentEdge->prob = fragments.pop();
}

/********************************************************************
 * System declaration
 */

void DocumentBuilder::instantiation_begin(std::string_view name, uint32_t parameters, std::string_view templ_name)
{
    // Make sure this identifier is new.
    if (frames.top().contains(name))
        handle_error(duplicate_definition_error(name));

    // Lookup symbol.
    auto id = Symbol{};
    if (!resolve(templ_name, id) ||
        (id.get_type().get_kind() != INSTANCE && id.get_type().get_kind() != LSC_INSTANCE)) {
        handle_error(TypeException{"$Not_a_template"});
    }

    // Push parameters to frame stack.
    auto frame = frames.top().make_sub();
    frame.add(params);
    push_frame(frame);
    params = Frame::make();
}

void DocumentBuilder::instantiation_end(std::string_view name, uint32_t parameters, std::string_view templ_name,
                                        uint32_t arguments)
{
    // Parameters are at the top of the frame stack.
    auto params = pop_top(frames);
    assert(parameters == params.get_size());

    /* Lookup symbol. In case of failure, instantiationBegin already
     * reported the problem.
     */
    auto id = Symbol{};
    if (resolve(templ_name, id) && (id.get_type().get_kind() == INSTANCE || id.get_type().get_kind() == LSC_INSTANCE)) {
        auto* old_instance = static_cast<Instance*>(id.get_data());
        // Check number of arguments. If too many arguments, pop the rest.
        auto expected = id.get_type().size();
        if (arguments < expected) {
            handle_error(TypeException{"$Too_few_arguments"});
        } else if (arguments > expected) {
            handle_error(TypeException{"$Too_many_arguments"});
        } else {
            // Collect arguments from expression stack.
            auto exprs = std::vector<Expression>(arguments);
            while (arguments-- > 0)
                exprs[arguments] = fragments.pop();
            arguments = 0;
            // Create template composition.
            Instance& new_instance = (id.get_type().get_kind() == INSTANCE)
                                         ? document.add_instance(name, *old_instance, params, exprs, position)
                                         : document.add_LSC_instance(name, *old_instance, params, exprs, position);

            /* Propagate information about restricted variables. The
             * variables used in arguments to restricted parameters of
             * old_instance are restricted in new_instance.
             *
             * REVISIT: Move to document.cpp?
             */
            std::set<Symbol>& restricted = old_instance->restricted;
            for (uint32_t i = 0; i < expected; i++) {
                if (restricted.find(old_instance->parameters[i]) != restricted.end()) {
                    collect_dependencies(new_instance.restricted, exprs[i]);
                }
            }
        }
    }
    fragments.pop(arguments);
}

// Adds process_t* pointer to system_line
// Checks for duplicate entries
void DocumentBuilder::process(std::string_view name)
{
    auto symbol = Symbol{};
    if (!resolve(name, symbol))
        throw no_such_process_error(name);

    auto type = symbol.get_type();
    if (type.get_kind() != INSTANCE)
        throw not_a_template_error(symbol.get_name());
    if (type.size() > 0) {
        // FIXME: Check type of unbound parameters
    }
    auto& instance = *static_cast<Instance*>(symbol.get_data());
    instance.templ->is_instantiated = true;
    document.add_process(instance, position);
    proc_priority(name);
}

void DocumentBuilder::process_list_end() {}

void DocumentBuilder::done() {}

void DocumentBuilder::before_update() { document.set_before_update(fragments.pop()); }

void DocumentBuilder::after_update() { document.set_after_update(fragments.pop()); }

/********************************************************************
 * Priority
 */

void DocumentBuilder::chan_priority_begin() { document.begin_chan_priority(fragments.pop()); }

void DocumentBuilder::chan_priority_add(char separator) { document.add_chan_priority(separator, fragments.pop()); }

void DocumentBuilder::chan_priority_default() { fragments.push(Expression()); }

void DocumentBuilder::proc_priority_inc() { currentProcPriority++; }

void DocumentBuilder::proc_priority(std::string_view name)
{
    Symbol symbol;
    if (!resolve(name, symbol)) {
        handle_error(no_such_process_error(name));
    } else {
        document.set_proc_priority(name, currentProcPriority);
    }
}

/**
 * Adds an instance line to the current template.
 */
void DocumentBuilder::proc_instance_line() { currentInstanceLine = &currentTemplate->add_instance_line(); }
/**
 * templ is true if the name of the instance contains parameters like
 * "Train(1)". Here, "Train" is the name of a template.
 *
 * Otherwise the name of the instance can be for example "Train1", where
 * "Train1" is a process that has been instantiated "Train1 = Train(1)".
 * In this case, we cannot check that "Train1" exists yet,
 * because the system declarations have not yet been parsed.
 */
void DocumentBuilder::instance_name(std::string_view name, bool templ)
{
    Symbol uid;
    if (templ) {
        auto instName = std::string{name};
        auto templName = instName.substr(0, instName.find('('));
        if (!resolve(templName, uid) || (uid.get_type().get_kind() != INSTANCE))
            handle_error(not_a_template_error(templName));
    } else {
        if (resolve(name, uid) && (uid.get_type().get_kind() == INSTANCE)) {
            const auto* t = static_cast<Template*>(uid.get_data());
            if (t->parameters.get_size() > 0) {
                handle_error(TypeException{"$Wrong_number_of_arguments_in_instance_line_name"});
            }
        }
    }
    currentInstanceLine->uid =
        currentTemplate->frame.add_symbol(name, Type::create_primitive(INSTANCE_LINE), position, currentInstanceLine);
}

void DocumentBuilder::instance_name_begin(std::string_view name)
{
    // Push parameters to frame stack.
    auto frame = frames.top().make_sub();
    frame.add(params);
    push_frame(frame);
    params = Frame::make();
}

void DocumentBuilder::instance_name_end(std::string_view name, uint32_t arguments)
{
    auto i_name = std::ostringstream{};
    i_name << name;
    // Parameters are at the top of the frame stack.
    auto params = pop_top(frames);
    // assert(parameters == params.get_size());

    // Lookup symbol. In case of failure, instance_name_begin already reported the problem.
    if (auto id = Symbol{}; resolve(name, id) && id.get_type().get_kind() == INSTANCE) {
        auto* old_instance = static_cast<Instance*>(id.get_data());

        /* Check number of arguments. If too many arguments, pop the
         * rest.
         */
        auto expected = id.get_type().size();
        if (arguments < expected) {
            handle_error(TypeException{"$Too_few_arguments"});
        } else if (arguments > expected) {
            handle_error(TypeException{"$Too_many_arguments"});
        } else {
            // Collect arguments from expression stack.
            auto exprs = std::vector<Expression>(arguments);
            while (arguments-- > 0)
                exprs[arguments] = fragments.pop();
            arguments = 0;
            i_name << '(' << infix(exprs, ",") << ')';
            instance_name(i_name.str().c_str());
            // Create template composition.
            currentInstanceLine->add_parameters(*old_instance, params, exprs);

            /* Propagate information about restricted variables. The
             * variables used in arguments to restricted parameters of
             * old_instance are restricted in new_instance.
             *
             * REVISIT: Move to document.cpp?
             */
            std::set<Symbol>& restricted = old_instance->restricted;
            for (uint32_t i = 0; i < expected; i++) {
                if (restricted.find(old_instance->parameters[i]) != restricted.end()) {
                    collect_dependencies(currentInstanceLine->restricted, exprs[i]);
                }
            }
        }
    }
    fragments.pop(arguments);
}

/**
 * Add a message to the current template.
 */
void DocumentBuilder::proc_message(std::string_view from, std::string_view to, const int loc, const bool pch)
{
    auto fid = Symbol{};
    auto tid = Symbol{};
    if (!resolve(from, fid) || !fid.get_type().is_instance_line()) {
        handle_error(TypeException{"$No_such_instance_line_(source)"});
    } else if (!resolve(to, tid) || !tid.get_type().is_instance_line()) {
        handle_error(TypeException{"$No_such_instance_line_(destination)"});
    } else {
        currentMessage = &currentTemplate->add_message(fid, tid, loc, pch);
    }
}

void DocumentBuilder::proc_message(Synchronisation type)  // Label
{
    if (currentMessage != nullptr)
        currentMessage->label = Expression::create_sync(fragments[0], type, position);
    fragments.pop();
}

/**
 * Add a condition to the current template.
 */
void DocumentBuilder::proc_condition(const std::vector<std::string>& anchors, const int loc, const bool pch,
                                     const bool hot)
{
    auto v_anchorid = std::vector<Symbol>{};
    auto error = false;
    auto isHot = hot;

    for (const auto& anchor : anchors) {
        Symbol anchorid;
        if (!resolve(anchor.c_str(), anchorid) || !anchorid.get_type().is_instance_line()) {
            handle_error(TypeException{"$No_such_instance_line_(anchor)"});
            error = true;
        } else if (!error) {
            v_anchorid.push_back(anchorid);
        }
    }
    if (pch && hot) {
        handle_warning(TypeException{"$In_the_prechart_all_conditions_are_cold"});
        isHot = false;
    }
    if (!error) {
        currentCondition = &currentTemplate->add_condition(v_anchorid, loc, pch, isHot);
        currentCondition->label = make_constant(1);
    }
}

void DocumentBuilder::proc_condition()
{
    if (currentCondition != nullptr)
        currentCondition->label = fragments[0];
    fragments.pop();
}

/**
 * Add an update to the current template.
 */
void DocumentBuilder::proc_LSC_update(std::string_view anchor, const int loc, const bool pch)
{
    auto anchorid = Symbol{};

    if (!resolve(anchor, anchorid) || !anchorid.get_type().is_instance_line()) {
        handle_error(TypeException{"$No_such_instance_line_(anchor)"});
    } else {
        currentUpdate = &currentTemplate->add_update(anchorid, loc, pch);
        currentUpdate->label = make_constant(1);
    }
}

void DocumentBuilder::proc_LSC_update()  // Label
{
    if (currentUpdate != nullptr)
        currentUpdate->label = fragments[0];
    fragments.pop();
}

void DocumentBuilder::prechart_set(const bool pch) { currentTemplate->has_prechart = pch; }

void DocumentBuilder::decl_dynamic_template(std::string_view name)
{
    // Should be null, but error recovery can result in proc_end not being called
    if (currentTemplate != nullptr) {
        currentTemplate = nullptr;
        declarations.pop();
    }
    // check if name already exists
    if (frames.top().contains(name))
        handle_error(duplicate_definition_error(name));

    for (uint32_t i = 0; i < params.get_size(); i++) {
        if (!params[i].get_type().is_integer() && !params[i].get_type().is_boolean() &&
            !(params[i].get_type().is(REF) && params[i].get_type()[0].is(BROADCAST)))
            handle_error(TypeException{"Dynamic template parameters can either be boolean or integer and "
                                       "cannot be references"});
    }

    document.add_dynamic_template(name, params, position);

    params = Frame::make();  // reset params
}

void DocumentBuilder::query_begin() { currentQuery = std::make_unique<Query>(); }
void DocumentBuilder::query_formula(std::string_view formula, std::string_view location)
{
    currentQuery->formula = formula;
    currentQuery->location = location;
}
void DocumentBuilder::query_comment(std::string_view comment) { currentQuery->comment = comment; }
void DocumentBuilder::query_options(std::string_view key, std::string_view value)
{
    if (key.empty())
        handle_error(TypeException{"options tag found without attribute 'key'"});
    currentQuery->options.emplace_back(std::string{key}, std::string{value});
}

void DocumentBuilder::expectation_begin() { currentExpectation = std::make_unique<Expectation>(); }

void DocumentBuilder::expectation_end()
{
    currentQuery->expectation = std::move(*currentExpectation);
    currentExpectation.reset();
}

void DocumentBuilder::expectation_value(std::string_view res, std::string_view type, std::string_view value)
{
    auto _type = ExpectationKind{};
    if (type.empty()) {
        _type = ExpectationKind::_ErrorValue;
    } else if (type == "probability"sv) {
        _type = ExpectationKind::Probability;
    } else if (type == "symbolic"sv) {
        _type = ExpectationKind::Symbolic;
    } else if (type == "value"sv) {
        _type = ExpectationKind::NumericValue;
    } else {
        _type = ExpectationKind::_ErrorValue;
    }
    if (res.empty()) {
        currentExpectation->status = QueryStatus::Unknown;
    } else if (res == "success"sv) {
        currentExpectation->status = QueryStatus::True;
    } else if (res == "failure"sv) {
        currentExpectation->status = QueryStatus::False;
    } else if (res == "maybe_true"sv) {
        currentExpectation->status = QueryStatus::MaybeTrue;
    } else if (res == "maybe_false"sv) {
        currentExpectation->status = QueryStatus::MaybeFalse;
    } else {
        currentExpectation->status = QueryStatus::Unknown;
    }
    currentExpectation->value_type = _type;
    currentExpectation->value = value;
}

void DocumentBuilder::expect_resource(std::string_view type, std::string_view value, std::string_view unit)
{
    if (type.empty()) {
        handle_error(TypeException{"missing name of resource in expectation"});
        return;
    }
    if (value.empty()) {
        handle_error(TypeException{"missing value of resource in expectation"});
    }
    currentExpectation->resources.emplace_back(std::string{type}, std::string{value},
                                               unit.empty() ? std::nullopt : std::make_optional<std::string>(unit));
}

void DocumentBuilder::query_results_begin() {}

void DocumentBuilder::query_results_end() { currentResults = nullptr; }

void DocumentBuilder::query_end()
{
    document.add_query(std::move(*currentQuery));
    currentQuery.reset();
}

void DocumentBuilder::model_option(std::string_view key, std::string_view value)
{
    if (key.empty())
        handle_error(TypeException{"options tag found without attribute 'key'"});
    document.get_options().emplace_back(std::string{key}, std::string{value});
}
