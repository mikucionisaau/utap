// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2004 Uppsala University and Aalborg University.

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

#ifndef UTAP_SYSTEMBUILDER_H
#define UTAP_SYSTEMBUILDER_H

#include "utap/StatementBuilder.hpp"
#include "utap/utap.hpp"

#include <vector>
#include <cinttypes>

namespace UTAP {
/**
 * This class constructs a Document. It avoids as much
 * type checking as possible - type checking should be done with
 * the TypeChecker class. However some checks are more convenient
 * to do in DocumentBuilder:
 *
 *  - states are not both committed and urgent
 *  - the source and target of an edge is a state
 *  - the dot operator is applied to a process or a structure (StatementBuilder)
 *  - functions are not recursive (StatementBuilder)
 *  - only declared functions are called (ExpressionBuilder)
 *  - functions are called with the correct number of arguments (StatementBuilder)
 *  - the initial state of a template is actually declared as a state
 *  - templates in instantiations have been declared
 *  - identifiers are not declared twice in the same scope (StatementBuilder)
 *  - type names do exist and are declared as types (StatementBuilder)
 *  - processes in the system line have been declared
 *
 * Left hand side expressions are assigned the correct type by
 * DocumentBuilder; if not it would be difficult to represent
 * dot-expressions.
 *
 * DocumentBuilder does not
 *  - check the correctness of variable initialisers, nor
 *    does it complete variable initialisers
 *  - type check expressions
 *  - check if arguments to functions or templates match the
 *    formal parameters
 *  - check if something is a proper left hand side value
 *  - check if things are assignment compatible
 *  - check conflicting use of synchronisations and guards on edge
 *  - handle properties
 *
 * Use TypeChecker for these things.
 */
class DocumentBuilder : public StatementBuilder
{
protected:
    /** The current process priority level. */
    int32_t currentProcPriority{0};

    /** The edge under construction. */
    Edge* currentEdge{nullptr};

    /** The gantt map under construction. */
    std::unique_ptr<GanttEntry> currentGantt{nullptr};

    /** The condition under construction. */
    LSCCondition* currentCondition{nullptr};

    /** The update under construction. */
    LSCUpdate* currentUpdate{nullptr};
    /** The message under construction. */
    LSCMessage* currentMessage{nullptr};
    /** The instance line under construction. */
    LSCInstanceLine* currentInstanceLine{nullptr};

    std::unique_ptr<IODecl> currentIODecl{nullptr};
    std::unique_ptr<Query> currentQuery{nullptr};

    std::unique_ptr<QueryResults> currentResults{nullptr};
    std::unique_ptr<Expectation> currentExpectation;

    Declarations* get_current_declaration_block();

    Variable* add_variable(Type type, std::string_view name, Expression init, position_t pos) override;
    bool add_function(Type type, std::string_view name, position_t pos) override;

    std::stack<Declarations*> declarations;

    void add_select_symbol_to_frame(std::string_view name, Frame&, position_t pos);

public:
    DocumentBuilder(Document&, std::vector<std::filesystem::path> paths = {});

    void gantt_decl_begin(std::string_view name) override;
    void gantt_decl_select(std::string_view id) override;
    void gantt_decl_end() override;
    void gantt_entry_begin() override;
    void gantt_entry_select(std::string_view id) override;
    void gantt_entry_end() override;
    void decl_progress(bool) override;
    void proc_begin(std::string_view name, const bool isTA = true, std::string_view type = {},
                    std::string_view mode = {}) override;
    void proc_end() override;
    void proc_location(std::string_view name, bool hasInvariant, bool hasER) override;
    void proc_location_commit(std::string_view name) override;
    void proc_location_urgent(std::string_view name) override;
    void proc_location_init(std::string_view name) override;
    void proc_branchpoint(std::string_view name) override;
    void proc_edge_begin(std::string_view from, std::string_view to, const bool control,
                         std::string_view actname) override;
    void proc_edge_end(std::string_view from = {}, std::string_view to = {}) override;
    void proc_select(std::string_view id) override;
    void proc_guard() override;
    void proc_sync(Constants::Synchronisation type) override;
    void proc_update() override;
    void proc_prob() override;
    void instantiation_begin(std::string_view, uint32_t, std::string_view) override;
    void instantiation_end(std::string_view, uint32_t, std::string_view, uint32_t) override;
    void process(std::string_view) override;
    void process_list_end() override;
    void done() override;
    void before_update() override;
    void after_update() override;
    void chan_priority_begin() override;
    void chan_priority_add(char separator) override;
    void chan_priority_default() override;
    void proc_priority_inc() override;
    void proc_priority(std::string_view) override;
    void proc_instance_line() override;
    void instance_name(std::string_view name, bool templ = true) override;
    void instance_name_begin(std::string_view name) override;
    void instance_name_end(std::string_view name, uint32_t arguments) override;
    void proc_message(std::string_view from, std::string_view to, const int loc, const bool pch) override;
    void proc_message(Constants::Synchronisation type) override;
    void proc_condition(const std::vector<std::string>& anchors, const int loc, const bool pch,
                        const bool hot) override;
    void proc_condition() override;  // Label
    void proc_LSC_update(std::string_view anchor, const int loc, const bool pch) override;
    void proc_LSC_update() override;  // Label
    void prechart_set(const bool pch) override;

    void decl_dynamic_template(std::string_view name) override;

    void model_option(std::string_view key, std::string_view value) override;
    void query_begin() override;
    void query_formula(std::string_view formula, std::string_view location) override;
    void query_comment(std::string_view comment) override;
    void query_options(std::string_view key, std::string_view value) override;
    void expectation_begin() override;
    void expectation_end() override;
    void expectation_value(std::string_view res, std::string_view type, std::string_view value) override;
    void expect_resource(std::string_view type, std::string_view value, std::string_view unit) override;
    void query_results_begin() override;
    void query_results_end() override;
    void query_end() override;

    void system_decl_begin() override;
    void system_decl_end() override;
};
}  // namespace UTAP
#endif
