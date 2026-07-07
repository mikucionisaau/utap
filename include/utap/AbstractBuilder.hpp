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

#ifndef UTAP_ABSTRACTBUILDER_HH
#define UTAP_ABSTRACTBUILDER_HH

#include "utap/builder.hpp"
#include "utap/position.hpp"
#include "utap/utap.hpp"

#include <exception>
#include <string>
#include <vector>

namespace UTAP {
class NotSupportedException : public std::logic_error
{
public:
    NotSupportedException(const std::string& fn): std::logic_error{fn + " is not supported"} {}
};

class AbstractBuilder : public ParserBuilder
{
protected:
    position_t position;

public:
    AbstractBuilder() = default;

    void set_position(uint32_t, uint32_t) override;

    /************************************************************
     * Query functions
     */
    bool is_type(std::string_view) override;

    /************************************************************
     * Types
     */
    void type_duplicate() override;
    void type_pop() override;
    void type_bool(TypePrefix) override;
    void type_int(TypePrefix) override;
    void type_string(TypePrefix) override;
    void type_double(TypePrefix) override;
    void type_bounded_int(TypePrefix) override;
    void type_channel(TypePrefix) override;
    void type_clock(TypePrefix) override;
    void type_void() override;
    void type_scalar(TypePrefix) override;
    void type_name(TypePrefix, std::string_view name) override;
    void type_struct(TypePrefix, uint32_t fields) override;
    void type_array_of_size(uint32_t) override;
    void type_array_of_type(uint32_t) override;
    void struct_field(std::string_view name) override;
    void decl_typedef(std::string_view name) override;

    /************************************************************
     * Variable declarations
     */
    void decl_var(std::string_view name, bool init) override;
    void decl_init_list(uint32_t num) override;            // n initialisers
    void decl_field_init(std::string_view name) override;  // 1 initialiser

    /********************************************************************
     * Gantt chart declaration
     */
    void gantt_decl_begin(std::string_view name) override;
    void gantt_decl_select(std::string_view id) override;
    void gantt_decl_end() override;
    void gantt_entry_begin() override;
    void gantt_entry_select(std::string_view id) override;
    void gantt_entry_end() override;

    /************************************************************
     * Progress measure declarations
     */
    void decl_progress(bool) override;

    /************************************************************
     * Function declarations
     */
    void decl_parameter(std::string_view name, bool) override;
    void func_type() override;
    void decl_func_begin(std::string_view name) override;  // n paramaters
    void decl_func_end() override;                         // 1 block
    void dynamic_load_lib(std::string_view name) override;
    void decl_external_func(std::string_view name, std::string_view alias) override;

    /************************************************************
     * Process declarations
     */
    void proc_begin(std::string_view name, const bool isTA = true, std::string_view type = "",
                    std::string_view mode = "") override;
    void proc_end() override;                                                           // 1 ProcBody
    void proc_location(std::string_view name, bool hasInvariant, bool hasER) override;  // 1 expr
    void proc_location_commit(std::string_view name) override;                          // mark previously decl. state
    void proc_location_urgent(std::string_view name) override;                          // mark previously decl. state
    void proc_location_init(std::string_view name) override;                            // mark previously decl. state
    void proc_branchpoint(std::string_view name) override;
    void proc_edge_begin(std::string_view from, std::string_view to, const bool control,
                         std::string_view actname) override;
    void proc_edge_end(std::string_view from, std::string_view to) override;
    // 1 epxr,1sync,1expr
    void proc_select(std::string_view id) override;
    void proc_guard() override;
    void proc_sync(Constants::Synchronisation type) override;  // 1 expr
    void proc_update() override;
    void proc_prob() override;
    /************************************************************
     * Process declarations for LSC
     */
    void proc_instance_line() override;
    void instance_name(std::string_view name, bool templ = true) override;
    void instance_name_begin(std::string_view name) override;
    void instance_name_end(std::string_view name, uint32_t arguments) override;
    void proc_message(std::string_view from, std::string_view to, const int loc, const bool pch) override;
    void proc_message(Constants::Synchronisation type) override;  // 1 expr
    void proc_condition(const std::vector<std::string>& anchors, const int loc, const bool pch,
                        const bool hot) override;
    void proc_condition() override;  // Label
    void proc_LSC_update(std::string_view anchor, const int loc, const bool pch) override;
    void proc_LSC_update() override;  // Label
    void prechart_set(const bool pch) override;

    /************************************************************
     * Statements
     */
    void block_begin() override;
    void block_end() override;
    void empty_statement() override;
    void for_begin() override;                             // 3 expr
    void for_end() override;                               // 1 stat
    void iteration_begin(std::string_view name) override;  // 1 id, 1 type
    void iteration_end(std::string_view name) override;    // 1 stat
    void while_begin() override;
    void while_end() override;  // 1 expr, 1 stat
    void do_while_begin() override;
    void do_while_end() override;  // 1 stat, 1 expr
    void if_begin() override;
    void if_condition() override;  // 1 expr
    void if_then() override;       // 1 statement
    void if_end(bool) override;    // 1 expr, 1 or 2 statements
    void break_statement() override;
    void continue_statement() override;
    void switch_begin() override;
    void switch_end() override;  // 1 expr, 1+ case/default
    void case_begin() override;
    void case_end() override;  // 1 expr, 0+ stat
    void default_begin() override;
    void default_end() override;           // 0+ statements
    void expr_statement() override;        // 1 expr
    void return_statement(bool) override;  // 1 expr if argument is true
    void assert_statement() override;

    /************************************************************
     * Expressions
     */
    void expr_true() override;
    void expr_false() override;
    void expr_double(double) override;
    void expr_string(std::string_view) override;
    void expr_location() override;
    void expr_identifier(std::string_view varName) override;
    void expr_nat(int32_t) override;  // natural number
    void expr_call_begin() override;
    void expr_call_end(uint32_t n) override;              // n exprs as arguments
    void expr_array() override;                           // 2 expr
    void expr_post_increment() override;                  // 1 expr
    void expr_pre_increment() override;                   // 1 expr
    void expr_post_decrement() override;                  // 1 expr
    void expr_pre_decrement() override;                   // 1 expr
    void expr_assignment(Constants::Kind op) override;    // 2 expr
    void expr_unary(Constants::Kind unaryop) override;    // 1 expr
    void expr_binary(Constants::Kind binaryop) override;  // 2 expr
    void expr_nary(Constants::Kind, uint32_t num) override;
    void expr_scenario(std::string_view name) override;
    void expr_ternary(Constants::Kind ternaryop, bool firstMissing) override;  // 3 expr
    void expr_inline_if() override;                                            // 3 expr
    void expr_comma() override;                                                // 2 expr
    void expr_dot(std::string_view) override;                                  // 1 expr
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
    void expr_proba_expected(std::string_view identifier) override;
    void expr_simulate(int no_of_exprs, bool = false, int = 0) override;
    void expr_builtin_function1(Constants::Kind) override;
    void expr_builtin_function2(Constants::Kind) override;
    void expr_builtin_function3(Constants::Kind) override;
    void expr_optimize_exp(Constants::Kind, PRICETYPE, Constants::Kind) override;
    void expr_load_strategy() override;
    void expr_save_strategy(std::string_view strategy_name) override;

    // MITL
    void expr_MITL_formula() override;
    void expr_MITL_until(int, int) override;
    void expr_MITL_release(int, int) override;
    void expr_MITL_disj() override;
    void expr_MITL_conj() override;
    void expr_MITL_next() override;
    void expr_MITL_atom() override;
    void expr_optimize(int, int, int, int) override;

    /************************************************************
     * System declaration
     */
    void instantiation_begin(std::string_view, uint32_t, std::string_view) override;
    void instantiation_end(std::string_view, uint32_t, std::string_view, uint32_t) override;
    void process(std::string_view) override;
    void process_list_end() override;
    void done() override;

    void handle_expect(std::string_view text) override;

    /************************************************************
     * Properties
     */
    void property() override;
    void scenario(std::string_view) override;              // LSC
    void parse(const char*) override;                      // LSC
    void strategy_declaration(std::string_view) override;  // tiga-smc
    void subjection(std::string_view) override;
    void imitation(std::string_view) override;

    /********************************************************************
     * Guiding
     */
    void before_update() override;
    void after_update() override;

    /********************************************************************
     * Priority
     */
    void chan_priority_begin() override;
    void chan_priority_add(char separator) override;
    void chan_priority_default() override;
    void proc_priority_inc() override;
    void proc_priority(std::string_view) override;

    void decl_dynamic_template(std::string_view name) override;
    void expr_spawn(int) override;
    void expr_exit() override;
    void expr_numof() override;

    void expr_forall_dynamic_begin(std::string_view, std::string_view) override;
    void expr_forall_dynamic_end(std::string_view name) override;
    void expr_exists_dynamic_begin(std::string_view, std::string_view) override;
    void expr_exists_dynamic_end(std::string_view name) override;
    void expr_sum_dynamic_begin(std::string_view, std::string_view) override;
    void expr_sum_dynamic_end(std::string_view) override;
    void expr_foreach_dynamic_begin(std::string_view, std::string_view) override;
    void expr_foreach_dynamic_end(std::string_view name) override;
    void expr_dynamic_process_expr(std::string_view) override;
    void expr_MITL_forall_dynamic_begin(std::string_view, std::string_view) override;
    void expr_MITL_forall_dynamic_end(std::string_view name) override;
    void expr_MITL_exists_dynamic_begin(std::string_view, std::string_view) override;
    void expr_MITL_exists_dynamic_end(std::string_view name) override;

    /** Verification queries */
    void model_option(std::string_view key, std::string_view value) override;
    void query_begin() override;
    void query_formula(std::string_view formula, std::string_view location) override;
    void query_comment(std::string_view comment) override;
    void query_options(std::string_view options, std::string_view) override;
    void expectation_begin() override;
    void expectation_end() override;
    void expectation_value(std::string_view res, std::string_view type, std::string_view value) override;
    void expect_resource(std::string_view type, std::string_view value, std::string_view unit) override;
    void query_results_begin() override;
    void query_results_end() override;
    void query_end() override;

    void builtin_decl_end() override {}
    void system_decl_begin() override {}
    void system_decl_end() override {}
};
}  // namespace UTAP
#endif
