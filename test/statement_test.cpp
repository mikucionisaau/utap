// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020-2024 Aalborg University.

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

#include "utap/expression.hpp"
#include "utap/statement.hpp"

#include <doctest/doctest.h>

TEST_SUITE_BEGIN("Statements");

using namespace UTAP;
using namespace std::string_literals;

static const std::string indent = INDENT;  // one indentation to harden the tests

TEST_CASE("Empty")
{
    auto s = EmptyStatement{};
    CHECK(s.to_string(indent) == indent + ";");
}

TEST_CASE("Composite")
{
    auto int_type = Type::create_primitive(UTAP::Constants::INT);
    auto global = Frame::make();
    auto var_a = global.add_symbol("a", int_type, {});
    auto id_a = Expression::create_identifier(var_a);
    auto val0 = Expression::create_constant(0);
    auto val1 = Expression::create_constant(1);
    auto val5 = Expression::create_constant(5);
    SUBCASE("Trivial")
    {
        auto val2 = Expression::create_constant(2);
        auto plus = Expression::create_binary(Constants::PLUS, val1, val2);
        SUBCASE("Assignment")
        {
            auto assign = Expression::create_binary(UTAP::Constants::ASSIGN, id_a, plus);
            auto s = ExprStatement{assign};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "a = 1 + 2;");
        }
        SUBCASE("Equality")
        {
            auto equal = Expression::create_binary(UTAP::Constants::EQ, id_a, plus);
            auto s = ExprStatement{equal};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "a == 1 + 2;");
        }
        SUBCASE("Assertion")
        {
            auto equal = Expression::create_binary(UTAP::Constants::EQ, id_a, plus);
            auto s = AssertStatement{equal};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "assert(a == 1 + 2);");
        }
    }
    SUBCASE("Conditional")
    {
        auto var_i = global.add_symbol("i", int_type);
        auto id_i = Expression::create_identifier(var_i);
        auto step_i = Expression::create_unary(UTAP::Constants::PRE_INCREMENT, id_i);
        auto step_a = Expression::create_binary(UTAP::Constants::ASS_PLUS, id_a, id_i);
        auto cond_i = Expression::create_binary(UTAP::Constants::LT, id_i, val5);
        auto cond_a = Expression::create_binary(UTAP::Constants::LT, id_a, val5);
        SUBCASE("If")
        {
            auto s =
                IfStatement{cond_i, std::make_unique<ExprStatement>(step_i), std::make_unique<ExprStatement>(step_a)};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "if (i < 5)\n" + indent + INDENT + "++i;\n" + indent + "else\n" +
                                             indent + INDENT + "a += i;\n");
        }
        SUBCASE("For loop")
        {
            auto init = Expression::create_binary(UTAP::Constants::ASSIGN, id_i, val0);
            auto s = ForStatement{init, cond_i, step_i, std::make_unique<ExprStatement>(step_a)};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "for (i = 0; i < 5; ++i)\n"s + indent + INDENT + "a += i;\n");
        }
        SUBCASE("While loop")
        {
            auto s = WhileStatement{cond_i, std::make_unique<ExprStatement>(step_i)};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "while (i < 5)\n"s + indent + INDENT + "++i;\n");
        }
        SUBCASE("Do-While loop")
        {
            auto s = DoWhileStatement{std::make_unique<ExprStatement>(step_i), cond_i};
            CHECK(s.returns() == false);
            CHECK(s.to_string(indent) == indent + "do\n" + indent + INDENT + "++i;\n"s + indent + "while (i < 5);\n");
        }
    }
    SUBCASE("Ranged loop")
    {
        auto int_0_5 = Type::create_range(int_type, val0, val5);
        auto var_i = global.add_symbol("i", int_0_5);
        auto id_i = Expression::create_identifier(var_i);
        auto comp = Expression::create_binary(UTAP::Constants::ASS_PLUS, id_a, id_i);
        auto s = RangeStatement{var_i, global, std::make_unique<ExprStatement>(comp)};
        CHECK(s.returns() == false);
        CHECK(s.to_string(indent) == indent + "for (i : int[0,5])\n"s + indent + INDENT + "a += i;\n");
    }
    SUBCASE("Block")
    {
        auto var_i = global.add_symbol("i", int_type);
        auto id_i = Expression::create_identifier(var_i);
        auto e1 = Expression::create_binary(UTAP::Constants::ASS_PLUS, id_a, id_i);
        auto e2 = Expression::create_unary(UTAP::Constants::PRE_INCREMENT, id_a);
        auto s = BlockStatement{global};
        CHECK(s.returns() == true);
        CHECK(s.to_string(indent) == "{\n" + indent + "}");
        s.push(std::make_unique<ExprStatement>(e1));
        CHECK(s.returns() == false);
        CHECK(s.to_string(indent) == "{\n" + indent + INDENT + "a += i;\n" + indent + "}");
        s.push(std::make_unique<ExprStatement>(e2));
        CHECK(s.returns() == false);
        CHECK(s.to_string(indent) == "{\n" + indent + INDENT + "a += i;\n" + indent + INDENT + "++a;\n" + indent + "}");
        s.push(std::make_unique<ReturnStatement>());
        CHECK(s.returns() == true);
        CHECK(s.to_string(indent) == "{\n" + indent + INDENT + "a += i;\n" + indent + INDENT + "++a;\n" + indent +
                                         INDENT + "return ;\n" + indent + "}");
    }
    SUBCASE("External block")
    {
        auto fn = [] { return true; };
        auto s = ExternalBlockStatement{global, (void*)+fn, true};
        CHECK(s.returns() == true);
        CHECK(s.to_string(indent) == "{\n" + indent + "}");
    }
    SUBCASE("Switch")
    {
        auto s = SwitchStatement{id_a};
        CHECK(s.returns() == false);
        CHECK(s.to_string(indent) == indent + "switch (a) ;\n");
        auto case0 = std::make_unique<CaseStatement>(val0);
        case0->stat = std::make_unique<BreakStatement>();
        s.push(std::move(case0));
        CHECK(s.to_string(indent) ==
              indent + "switch (a)\n" + indent + INDENT + "case 0:\n" + indent + INDENT + INDENT + "break;\n");
        auto case1 = std::make_unique<CaseStatement>(val1);
        case1->stat = std::make_unique<BreakStatement>();
        s.push(std::move(case1));
        CHECK(s.to_string(indent) == indent + "switch (a) {\n" + indent + INDENT + "case 0:\n" + indent + INDENT +
                                         INDENT + "break;\n" + indent + INDENT + "case 1:\n" + indent + INDENT +
                                         INDENT + "break;\n" + indent + "}\n");
        auto def = std::make_unique<DefaultStatement>();
        def->stat = std::make_unique<BreakStatement>();
        s.push(std::move(def));
        CHECK(s.to_string(indent) == indent + "switch (a) {\n" + indent + INDENT + "case 0:\n" + indent + INDENT +
                                         INDENT + "break;\n" + indent + INDENT + "case 1:\n" + indent + INDENT +
                                         INDENT + "break;\n" + indent + INDENT + "default:\n" + indent + INDENT +
                                         INDENT + "break;\n" + indent + "}\n");
    }
    SUBCASE("Return")
    {
        auto s = ReturnStatement{id_a};
        CHECK(s.returns() == true);
        CHECK(s.to_string(indent) == indent + "return a;");
    }
}

TEST_SUITE_END();
