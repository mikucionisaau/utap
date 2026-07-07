// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002 Uppsala University and Aalborg University.

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

#ifndef UTAP_STATEMENT_HPP
#define UTAP_STATEMENT_HPP

#include "utap/document.hpp"
#include "utap/expression.hpp"
#include "utap/symbols.hpp"

#include <memory>
#include <string>
#include <utility>
#include <cstdint>

namespace UTAP {

constexpr auto INDENT = "\t";
class StatementVisitor;

class Statement
{
public:
    virtual ~Statement() noexcept = default;
    virtual int32_t accept(StatementVisitor&) = 0;
    virtual bool returns() const = 0;
    virtual std::ostream& print(std::ostream&, const std::string& indent = {}) const = 0;
    std::string to_string(const std::string& indent = {}) const;
    friend std::ostream& operator<<(std::ostream& os, const Statement& s) { return s.print(os); }

protected:
    Statement() = default;
};

class EmptyStatement final : public Statement
{
public:
    EmptyStatement() = default;
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class ExprStatement final : public Statement
{
public:
    Expression expr;
    explicit ExprStatement(Expression expr): expr{std::move(expr)} {}
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class AssertStatement final : public Statement
{
public:
    Expression expr;
    explicit AssertStatement(Expression expr): expr{std::move(expr)} {}
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class IfStatement final : public Statement
{
public:
    Expression cond;
    std::unique_ptr<Statement> trueCase;
    std::unique_ptr<Statement> falseCase;
    IfStatement(Expression cond, std::unique_ptr<Statement> trueCase, std::unique_ptr<Statement> falseCase = nullptr);
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return trueCase->returns() && falseCase != nullptr && falseCase->returns(); }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class ForStatement final : public Statement
{
public:
    Expression init;
    Expression cond;
    Expression step;
    std::unique_ptr<Statement> stat;
    ForStatement(Expression init, Expression cond, Expression step, std::unique_ptr<Statement> statement);
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

/// Statement class for the ranged-loop construction.
class RangeStatement final : public Statement
{
private:
    Frame frame;

public:
    Symbol symbol;
    std::unique_ptr<Statement> stat;
    RangeStatement(Symbol symbol, Frame frame, std::unique_ptr<Statement> statement):
        frame{std::move(frame)}, symbol{std::move(symbol)}, stat{std::move(statement)}
    {}
    const Frame& get_frame() const { return frame; }
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class WhileStatement final : public Statement
{
public:
    Expression cond;
    std::unique_ptr<Statement> stat;
    WhileStatement(Expression condition, std::unique_ptr<Statement> statement);
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class DoWhileStatement final : public Statement
{
public:
    std::unique_ptr<Statement> stat;
    Expression cond;
    DoWhileStatement(std::unique_ptr<Statement>, Expression);
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return stat->returns(); }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

/// Abstract class containing loose collection of statements without additional frame
class CompositeStatement : public Statement
{
public:
    using const_iterator = std::vector<std::unique_ptr<Statement>>::const_iterator;
    using iterator = std::vector<std::unique_ptr<Statement>>::iterator;
    CompositeStatement() = default;
    std::ostream& print(std::ostream&, const std::string& indent) const override;
    bool returns() const override { return empty() || back().returns(); }
    /// pushes the statement to the end of statement list
    void push(std::unique_ptr<Statement> stat);
    /// pops the last statement from the list
    std::unique_ptr<Statement> pop();
    /// returns the last statement in the list
    const Statement& back() const;
    /// returns the last statement in the list
    Statement& back();
    const_iterator begin() const { return stats.begin(); }
    const_iterator end() const { return stats.end(); }
    bool empty() const { return stats.empty(); }
    iterator begin() { return stats.begin(); }
    iterator end() { return stats.end(); }

protected:
    std::vector<std::unique_ptr<Statement>> stats;
};

/// Collection of statements with its own declarations and frame.
class BlockStatement : public CompositeStatement
{
    Declarations decl;

public:
    explicit BlockStatement(Frame frame) { decl.frame = std::move(frame); }
    int32_t accept(StatementVisitor&) override;
    std::ostream& print(std::ostream&, const std::string& indent) const override;
    const Frame& get_frame() const { return decl.frame; }
    Frame& get_frame() { return decl.frame; }
};

class ExternalBlockStatement final : public BlockStatement
{
public:
    ExternalBlockStatement(Frame frame, void* fp, bool ret):
        BlockStatement{std::move(frame)}, function{fp}, doesReturn{ret}
    {}
    bool returns() const override { return doesReturn; }
    void* getFP() { return function; }

private:
    void* function;
    bool doesReturn;
};

class SwitchStatement : public CompositeStatement
{
public:
    Expression cond;
    SwitchStatement(Expression expr): CompositeStatement{}, cond{std::move(expr)} {}
    int32_t accept(StatementVisitor& v) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class CaseStatement final : public Statement
{
public:
    Expression cond;
    std::unique_ptr<Statement> stat;
    CaseStatement(Expression value): cond{std::move(value)} {}
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class DefaultStatement final : public Statement
{
public:
    std::unique_ptr<Statement> stat;
    DefaultStatement() = default;
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class BreakStatement final : public Statement
{
public:
    BreakStatement() = default;
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class ContinueStatement final : public Statement
{
public:
    ContinueStatement() = default;
    int32_t accept(StatementVisitor&) override;
    bool returns() const override { return false; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class ReturnStatement final : public Statement
{
public:
    Expression value;
    ReturnStatement() = default;
    explicit ReturnStatement(Expression expr): value{std::move(expr)} {}
    int32_t accept(StatementVisitor& visitor) override;
    bool returns() const override { return true; }
    std::ostream& print(std::ostream&, const std::string& indent) const override;
};

class StatementVisitor
{
public:
    virtual ~StatementVisitor() noexcept = default;
    virtual int32_t visit_empty_statement(EmptyStatement& stat) = 0;
    virtual int32_t visit_expr_statement(ExprStatement& stat) = 0;
    virtual int32_t visit_assert_statement(AssertStatement& stat) = 0;
    virtual int32_t visit_for_statement(ForStatement& stat) = 0;
    virtual int32_t visit_iteration_statement(RangeStatement& stat) = 0;
    virtual int32_t visit_while_statement(WhileStatement& stat) = 0;
    virtual int32_t visit_do_while_statement(DoWhileStatement& stat) = 0;
    virtual int32_t visit_block_statement(BlockStatement& stat) = 0;
    virtual int32_t visit_switch_statement(SwitchStatement& stat) = 0;
    virtual int32_t visit_case_statement(CaseStatement& stat) = 0;
    virtual int32_t visit_default_statement(DefaultStatement& stat) = 0;
    virtual int32_t visit_if_statement(IfStatement& stat) = 0;
    virtual int32_t visit_break_statement(BreakStatement& stat) = 0;
    virtual int32_t visit_continue_statement(ContinueStatement& stat) = 0;
    virtual int32_t visit_return_statement(ReturnStatement& stat) = 0;
};

inline int32_t EmptyStatement::accept(StatementVisitor& v) { return v.visit_empty_statement(*this); }
inline int32_t ExprStatement::accept(StatementVisitor& v) { return v.visit_expr_statement(*this); }
inline int32_t AssertStatement::accept(StatementVisitor& v) { return v.visit_assert_statement(*this); }
inline int32_t ForStatement::accept(StatementVisitor& v) { return v.visit_for_statement(*this); }
inline int32_t RangeStatement::accept(StatementVisitor& v) { return v.visit_iteration_statement(*this); }
inline int32_t WhileStatement::accept(StatementVisitor& v) { return v.visit_while_statement(*this); }
inline int32_t DoWhileStatement::accept(StatementVisitor& v) { return v.visit_do_while_statement(*this); }
inline int32_t BlockStatement::accept(StatementVisitor& v) { return v.visit_block_statement(*this); }
inline int32_t SwitchStatement::accept(StatementVisitor& v) { return v.visit_switch_statement(*this); }
inline int32_t CaseStatement::accept(StatementVisitor& v) { return v.visit_case_statement(*this); }
inline int32_t DefaultStatement::accept(StatementVisitor& v) { return v.visit_default_statement(*this); }
inline int32_t IfStatement::accept(StatementVisitor& v) { return v.visit_if_statement(*this); }
inline int32_t BreakStatement::accept(StatementVisitor& v) { return v.visit_break_statement(*this); }
inline int32_t ContinueStatement::accept(StatementVisitor& v) { return v.visit_continue_statement(*this); }
inline int32_t ReturnStatement::accept(StatementVisitor& v) { return v.visit_return_statement(*this); }

/// Calls visit_statement on every statement
class AbstractStatementVisitor : public StatementVisitor
{
protected:
    virtual int32_t visit_statement(Statement& stat);
    virtual int32_t visit_composite_statement(CompositeStatement& stat);

public:
    int32_t visit_empty_statement(EmptyStatement& stat) override;
    int32_t visit_expr_statement(ExprStatement& stat) override;
    int32_t visit_assert_statement(AssertStatement& stat) override;
    int32_t visit_for_statement(ForStatement& stat) override;
    int32_t visit_iteration_statement(RangeStatement& stat) override;
    int32_t visit_while_statement(WhileStatement& stat) override;
    int32_t visit_do_while_statement(DoWhileStatement& stat) override;
    int32_t visit_block_statement(BlockStatement& stat) override;
    int32_t visit_switch_statement(SwitchStatement& stat) override;
    int32_t visit_case_statement(CaseStatement& stat) override;
    int32_t visit_default_statement(DefaultStatement& stat) override;
    int32_t visit_if_statement(IfStatement& stat) override;
    int32_t visit_break_statement(BreakStatement& stat) override;
    int32_t visit_continue_statement(ContinueStatement& stat) override;
    int32_t visit_return_statement(ReturnStatement& stat) override;
};

/// Calls visit_expression on every expression found in statements
class ExpressionVisitor : public AbstractStatementVisitor
{
protected:
    virtual void visit_expression(Expression&) = 0;

public:
    int32_t visit_expr_statement(ExprStatement& stat) override;
    int32_t visit_assert_statement(AssertStatement& stat) override;
    int32_t visit_for_statement(ForStatement& stat) override;
    int32_t visit_while_statement(WhileStatement& stat) override;
    int32_t visit_do_while_statement(DoWhileStatement& stat) override;
    int32_t visit_block_statement(BlockStatement& stat) override;
    int32_t visit_switch_statement(SwitchStatement& stat) override;
    int32_t visit_case_statement(CaseStatement& stat) override;
    int32_t visit_default_statement(DefaultStatement& stat) override;
    int32_t visit_if_statement(IfStatement& stat) override;
    int32_t visit_return_statement(ReturnStatement& stat) override;
};

/// Collects symbols that the statement changes
std::set<Symbol> collect_changes(Statement& stat);

/// Collects symbols that the statement reads
std::set<Symbol> collect_dependencies(Statement& stat);

/// Collects dynamic expressions within the statement
std::vector<Expression> collect_dynamic_expressions(Statement& stat);

}  // namespace UTAP
#endif  // UTAP_STATEMENT_HPP
