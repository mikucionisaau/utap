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

#include "utap/AbstractBuilder.hpp"

#include <vector>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>

using namespace UTAP;

/*
void ParserBuilder::handleWarning(const char* msg, ...)
{
    char str[256];
    va_list ap;
    va_start(ap, msg);
    vsnprintf(str, 256, msg, ap);
    va_end(ap);

    handle_warning(std::string(str));
}

void ParserBuilder::handleError(const char* msg, ...)
{
    char str[256];
    va_list ap;
    va_start(ap, msg);
    vsnprintf(str, 256, msg, ap);
    va_end(ap);

    handle_error(std::string(str));
}
*/

void AbstractBuilder::set_position(uint32_t start, uint32_t end)
{
    position.start = start;
    position.end = end;
}

bool AbstractBuilder::is_type(std::string_view) { return false; }

#define UNSUPPORTED throw NotSupportedException(__FUNCTION__)

void AbstractBuilder::type_duplicate() { UNSUPPORTED; }
void AbstractBuilder::type_pop() { UNSUPPORTED; }
void AbstractBuilder::type_bool(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_int(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_string(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_double(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_bounded_int(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_channel(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_clock(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_void() { UNSUPPORTED; }
void AbstractBuilder::type_scalar(TypePrefix) { UNSUPPORTED; }
void AbstractBuilder::type_name(TypePrefix, std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::type_struct(TypePrefix, uint32_t fields) { UNSUPPORTED; }
void AbstractBuilder::type_array_of_size(uint32_t) { UNSUPPORTED; }
void AbstractBuilder::type_array_of_type(uint32_t) { UNSUPPORTED; }
void AbstractBuilder::struct_field(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::decl_typedef(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::decl_var(std::string_view name, bool init) { UNSUPPORTED; }
void AbstractBuilder::decl_init_list(uint32_t num) { UNSUPPORTED; }
void AbstractBuilder::decl_field_init(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::gantt_decl_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::gantt_decl_select(std::string_view id) { UNSUPPORTED; }
void AbstractBuilder::gantt_decl_end() { UNSUPPORTED; }
void AbstractBuilder::gantt_entry_begin() { UNSUPPORTED; }
void AbstractBuilder::gantt_entry_select(std::string_view id) { UNSUPPORTED; }
void AbstractBuilder::gantt_entry_end() { UNSUPPORTED; }

void AbstractBuilder::decl_progress(bool) { UNSUPPORTED; }
void AbstractBuilder::decl_parameter(std::string_view name, bool) { UNSUPPORTED; }
void AbstractBuilder::func_type() { UNSUPPORTED; }
void AbstractBuilder::decl_func_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::decl_func_end() { UNSUPPORTED; }
void AbstractBuilder::dynamic_load_lib(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::decl_external_func(std::string_view name, std::string_view alias) { UNSUPPORTED; }

void AbstractBuilder::proc_begin(std::string_view name, const bool isTA, std::string_view type, std::string_view mode)
{
    UNSUPPORTED;
}

void AbstractBuilder::proc_end() { UNSUPPORTED; }
void AbstractBuilder::proc_location(std::string_view name, bool hasInvariant, bool hasER) { UNSUPPORTED; }
void AbstractBuilder::proc_location_commit(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::proc_location_urgent(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::proc_location_init(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::proc_branchpoint(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::proc_edge_begin(std::string_view from, std::string_view to, const bool control,
                                      std::string_view actname)
{
    UNSUPPORTED;
}

void AbstractBuilder::proc_edge_end(std::string_view from, std::string_view to) { UNSUPPORTED; }
void AbstractBuilder::proc_select(std::string_view id) { UNSUPPORTED; }
void AbstractBuilder::proc_guard() { UNSUPPORTED; }
void AbstractBuilder::proc_sync(Constants::Synchronisation type) { UNSUPPORTED; }
void AbstractBuilder::proc_update() { UNSUPPORTED; }
void AbstractBuilder::proc_prob() { UNSUPPORTED; }

// LSC
void AbstractBuilder::proc_message(Constants::Synchronisation type) { UNSUPPORTED; }
void AbstractBuilder::proc_instance_line() { UNSUPPORTED; }
void AbstractBuilder::instance_name_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::instance_name_end(std::string_view name, uint32_t arguments) { UNSUPPORTED; }
void AbstractBuilder::instance_name(std::string_view name, bool templ) { UNSUPPORTED; }

void AbstractBuilder::proc_message(std::string_view from, std::string_view to, const int loc, const bool pch)
{
    UNSUPPORTED;
}
void AbstractBuilder::proc_condition(const std::vector<std::string>& anchors, const int loc, const bool pch,
                                     const bool hot)
{
    UNSUPPORTED;
}
void AbstractBuilder::proc_condition() { UNSUPPORTED; }
void AbstractBuilder::proc_LSC_update(std::string_view anchor, const int loc, const bool pch) { UNSUPPORTED; }
void AbstractBuilder::proc_LSC_update() { UNSUPPORTED; }
void AbstractBuilder::prechart_set(const bool pch) { UNSUPPORTED; }
// end LSC

void AbstractBuilder::block_begin() { UNSUPPORTED; }
void AbstractBuilder::block_end() { UNSUPPORTED; }
void AbstractBuilder::empty_statement() { UNSUPPORTED; }
void AbstractBuilder::for_begin() { UNSUPPORTED; }
void AbstractBuilder::for_end() { UNSUPPORTED; }
void AbstractBuilder::iteration_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::iteration_end(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::while_begin() { UNSUPPORTED; }
void AbstractBuilder::while_end() { UNSUPPORTED; }
void AbstractBuilder::do_while_begin() { UNSUPPORTED; }
void AbstractBuilder::do_while_end() { UNSUPPORTED; }

void AbstractBuilder::if_begin() { UNSUPPORTED; }
void AbstractBuilder::if_condition() { UNSUPPORTED; }
void AbstractBuilder::if_then() { UNSUPPORTED; }
void AbstractBuilder::if_end(bool) { UNSUPPORTED; }

void AbstractBuilder::break_statement() { UNSUPPORTED; }
void AbstractBuilder::continue_statement() { UNSUPPORTED; }

void AbstractBuilder::switch_begin() { UNSUPPORTED; }
void AbstractBuilder::switch_end() { UNSUPPORTED; }
void AbstractBuilder::case_begin() { UNSUPPORTED; }
void AbstractBuilder::case_end() { UNSUPPORTED; }
void AbstractBuilder::default_begin() { UNSUPPORTED; }

void AbstractBuilder::default_end() { UNSUPPORTED; }
void AbstractBuilder::expr_statement() { UNSUPPORTED; }
void AbstractBuilder::return_statement(bool) { UNSUPPORTED; }
void AbstractBuilder::assert_statement() { UNSUPPORTED; }

void AbstractBuilder::expr_true() { UNSUPPORTED; }
void AbstractBuilder::expr_false() { UNSUPPORTED; }
void AbstractBuilder::expr_double(double) { UNSUPPORTED; }
void AbstractBuilder::expr_string(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_identifier(std::string_view varName) { UNSUPPORTED; }
void AbstractBuilder::expr_nat(int32_t) { UNSUPPORTED; }
void AbstractBuilder::expr_call_begin() { UNSUPPORTED; }
void AbstractBuilder::expr_call_end(uint32_t n) { UNSUPPORTED; }
void AbstractBuilder::expr_array() { UNSUPPORTED; }

void AbstractBuilder::expr_post_increment() { UNSUPPORTED; }
void AbstractBuilder::expr_pre_increment() { UNSUPPORTED; }
void AbstractBuilder::expr_post_decrement() { UNSUPPORTED; }
void AbstractBuilder::expr_pre_decrement() { UNSUPPORTED; }
void AbstractBuilder::expr_assignment(Constants::Kind op) { UNSUPPORTED; }
void AbstractBuilder::expr_unary(Constants::Kind unaryop) { UNSUPPORTED; }
void AbstractBuilder::expr_binary(Constants::Kind binaryop) { UNSUPPORTED; }
void AbstractBuilder::expr_nary(Constants::Kind kind, uint32_t num) { UNSUPPORTED; }

// LSC
void AbstractBuilder::expr_scenario(std::string_view name) { UNSUPPORTED; }
// end LSC

void AbstractBuilder::expr_ternary(Constants::Kind ternaryop, bool firstMissing) { UNSUPPORTED; }
void AbstractBuilder::expr_inline_if() { UNSUPPORTED; }
void AbstractBuilder::expr_comma() { UNSUPPORTED; }
void AbstractBuilder::expr_location() { UNSUPPORTED; }
void AbstractBuilder::expr_dot(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_deadlock() { UNSUPPORTED; }
void AbstractBuilder::expr_forall_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_forall_end(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_sum_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_sum_end(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::expr_builtin_function1(Constants::Kind) { UNSUPPORTED; }
void AbstractBuilder::expr_builtin_function2(Constants::Kind) { UNSUPPORTED; }
void AbstractBuilder::expr_builtin_function3(Constants::Kind) { UNSUPPORTED; }

void AbstractBuilder::expr_simulate(int no_of_exprs, bool, int) { UNSUPPORTED; }
void AbstractBuilder::expr_optimize_exp(Constants::Kind, PRICETYPE, Constants::Kind) { UNSUPPORTED; }
void AbstractBuilder::expr_load_strategy() { UNSUPPORTED; }
void AbstractBuilder::expr_save_strategy(std::string_view strategy_name) { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_formula() { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_until(int, int) { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_release(int, int) { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_disj() { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_conj() { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_next() { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_atom() { UNSUPPORTED; }

void AbstractBuilder::expr_optimize(int, int, int, int) { UNSUPPORTED; }
void AbstractBuilder::expr_proba_qualitative(Constants::Kind, Constants::Kind, double) { UNSUPPORTED; }
void AbstractBuilder::expr_proba_quantitative(Constants::Kind) { UNSUPPORTED; }
void AbstractBuilder::expr_proba_compare(Constants::Kind, Constants::Kind) { UNSUPPORTED; }
void AbstractBuilder::expr_proba_expected(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_exists_begin(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_exists_end(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::instantiation_begin(std::string_view, uint32_t, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::instantiation_end(std::string_view, uint32_t, std::string_view, uint32_t) { UNSUPPORTED; }

void AbstractBuilder::process(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::process_list_end() { UNSUPPORTED; }

// LSC
void AbstractBuilder::scenario(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::parse(const char*) { UNSUPPORTED; }
// end LSC

void AbstractBuilder::done() {}
void AbstractBuilder::handle_expect(std::string_view text) {}
void AbstractBuilder::property() { UNSUPPORTED; }

void AbstractBuilder::strategy_declaration(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::subjection(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::imitation(std::string_view) { UNSUPPORTED; }

void AbstractBuilder::before_update() { UNSUPPORTED; }
void AbstractBuilder::after_update() { UNSUPPORTED; }
void AbstractBuilder::chan_priority_begin() { UNSUPPORTED; }
void AbstractBuilder::chan_priority_add(char separator) { UNSUPPORTED; }
void AbstractBuilder::chan_priority_default() { UNSUPPORTED; }
void AbstractBuilder::proc_priority_inc() { UNSUPPORTED; }
void AbstractBuilder::proc_priority(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::decl_dynamic_template(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::expr_spawn(int) { UNSUPPORTED; }
void AbstractBuilder::expr_exit() { UNSUPPORTED; }
void AbstractBuilder::expr_numof() { UNSUPPORTED; }
void AbstractBuilder::expr_forall_dynamic_begin(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_forall_dynamic_end(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_exists_dynamic_begin(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_exists_dynamic_end(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_sum_dynamic_begin(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_sum_dynamic_end(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_foreach_dynamic_begin(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_foreach_dynamic_end(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_dynamic_process_expr(std::string_view) { UNSUPPORTED; }

void AbstractBuilder::expr_MITL_forall_dynamic_begin(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_forall_dynamic_end(std::string_view name) { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_exists_dynamic_begin(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expr_MITL_exists_dynamic_end(std::string_view name) { UNSUPPORTED; }

void AbstractBuilder::query_begin() { UNSUPPORTED; }
void AbstractBuilder::query_end() { UNSUPPORTED; }
void AbstractBuilder::query_formula(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::query_comment(std::string_view) { UNSUPPORTED; }
void AbstractBuilder::query_options(std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::expectation_begin() { UNSUPPORTED; }
void AbstractBuilder::expectation_end() { UNSUPPORTED; }
void AbstractBuilder::expectation_value(std::string_view type, std::string_view res, std::string_view value)
{
    UNSUPPORTED;
}
void AbstractBuilder::expect_resource(std::string_view, std::string_view, std::string_view) { UNSUPPORTED; }
void AbstractBuilder::query_results_begin() { UNSUPPORTED; }
void AbstractBuilder::query_results_end() { UNSUPPORTED; }
void AbstractBuilder::model_option(std::string_view, std::string_view) { UNSUPPORTED; }
