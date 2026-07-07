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

#ifndef UTAP_STATEMENTBUILDER_H
#define UTAP_STATEMENTBUILDER_H

#include "utap/ExpressionBuilder.hpp"
#include "utap/utap.hpp"

#include <filesystem>
#include <vector>
#include <cinttypes>

namespace UTAP {
/**
 * Partial implementation of the builder interface, useful
 * for building something with statements that is not a UTAP document.
 */
class StatementBuilder : public ExpressionBuilder
{
protected:
    /**
     * The \a params frame is used temporarily during parameter
     * parsing.
     */
    Frame params{Frame::make()};

    /** The function currently being parsed. */
    Function* currentFun{nullptr};

    /** Stack of nested statement blocks. */
    std::vector<std::unique_ptr<BlockStatement>> blocks;

    /** The types of a struct. */
    std::vector<Type> fields;

    /** The labels of a struct. */
    std::vector<std::string> labels;

    /** The positions of a struct. */
    std::vector<position_t> positions;

    /** path to libraries*/
    std::vector<std::filesystem::path> libpaths;

    virtual Variable* add_variable(Type type, std::string_view name, Expression init, position_t pos) = 0;
    virtual bool add_function(Type type, std::string_view name, position_t pos) = 0;

    static void collect_dependencies(std::set<Symbol>&, const Expression&);
    static void collect_dependencies(std::set<Symbol>&, const Type&);

public:
    explicit StatementBuilder(Document&, std::vector<std::filesystem::path> libpaths = {});
    StatementBuilder(const StatementBuilder&) = delete;

    BlockStatement& get_block();

    void type_array_of_size(uint32_t) override;
    void type_array_of_type(uint32_t) override;
    void type_struct(TypePrefix, uint32_t fields) override;
    void struct_field(std::string_view name) override;
    void decl_typedef(std::string_view name) override;
    void decl_var(std::string_view name, bool init) override;
    Expression make_initialiser(const Type& type, const Expression& init);
    void decl_init_list(uint32_t num) override;
    void decl_field_init(std::string_view name) override;
    void decl_parameter(std::string_view name, bool) override;
    void func_type() override;
    void decl_func_begin(std::string_view name) override;
    void decl_func_end() override;
    void dynamic_load_lib(std::string_view name) override;
    void decl_external_func(std::string_view name, std::string_view alias) override;
    void block_begin() override;
    void block_end() override;
    void empty_statement() override;
    void for_begin() override;
    void for_end() override;
    void iteration_begin(std::string_view name) override;
    void iteration_end(std::string_view name) override;
    void while_begin() override;
    void while_end() override;
    void do_while_begin() override;
    void do_while_end() override;
    void if_begin() override {};
    void if_condition() override {};
    void if_then() override {};
    void if_end(bool) override;
    void expr_statement() override;
    void return_statement(bool) override;
    void assert_statement() override;
    void expr_call_begin() override;
};
}  // namespace UTAP
#endif
