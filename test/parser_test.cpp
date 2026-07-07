// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 2021-2022, Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

/**
 * File:   test_parser.cpp
 * Author: Rasmus R. Kjær
 *
 * Created on 20 August 2021, 09:47
 */

#include "document_fixture.h"

#include "utap/Library.hpp"
#include "utap/StatementBuilder.hpp"
#include "utap/TypeChecker.hpp"
#include "utap/utap.hpp"

#include <doctest/doctest.h>

TEST_SUITE_BEGIN("parser");

using namespace UTAP;
using namespace std::string_literals;

TEST_CASE("Double Serialization Test")
{
    const auto doc = read_document("if_statement.xml");
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Power expressions")
{
    const auto doc = read_document("powers.xml");
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("External functions")
{
    using namespace UTAP;
    auto doc = read_document("external_fn.xml");
    const auto& errs = doc.get_errors();
    REQUIRE(errs.size() == 3);
    if constexpr (is(OS::Linux)) {
        CHECK(errs[0].msg == Contains{"libbad.so: cannot open shared object file: No such file or directory"});
        CHECK(errs[1].msg == Contains{"libbad.so: cannot open shared object file: No such file or directory"});
        CHECK(errs[2].msg == Contains{"undefined symbol: absent"});
    } else if constexpr (is(OS::Windows)) {
        CHECK(errs[0].msg == Contains{"Failed to open dynamic library libbad.dll: error 126: Module not found."});
        CHECK(errs[1].msg == Contains{"Failed to open dynamic library"});
        CHECK(errs[1].msg == Contains{"libbad.dll: error 126: Module not found."});
        CHECK(errs[2].msg == Contains{"Failed to find symbol: error 127: Procedure not found."});
    } else if constexpr (is(OS::macOS)) {
        CHECK(errs[0].msg == Contains{"no such file"});
        CHECK(errs[1].msg == Contains{"no such file"});
        CHECK(errs[2].msg == Contains{"symbol not found"});
    } else {
        REQUIRE_MESSAGE(false, "OS is not supported");
    }
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
    // TypeChecker is not run when errors are present, so we do it on our own:
    auto checker = TypeChecker{doc};
    doc.accept(checker);
    REQUIRE(errs.size() == 3);  // no new errors
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Error location")
{
    auto doc = read_document("smc_non-deterministic_input2.xml");
    const auto& errs = doc.get_errors();
    const auto& warns = doc.get_warnings();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
    const auto& templates = doc.get_templates();
    REQUIRE(templates.size() > 0);
    const auto& edges = templates.front().edges;
    REQUIRE(edges.size() > 0);
    const auto& pos = edges.front().sync.get_position();
    doc.add_error(pos, "$Non_deterministic_input", "c?");
    REQUIRE(errs.size() == 1);
    const auto& error = errs.front();
    CHECK(error.msg == "$Non_deterministic_input");
    REQUIRE(error.start.path != nullptr);
    CHECK(*error.start.path == "/nta/template[1]/transition[1]/label[1]");
}

TEST_CASE("SMC bounds in queries")
{
    auto doc = std::make_unique<Document>();
    auto builder = QueryBuilder{*doc};
    SUBCASE("Probability estimation query with 7 runs")
    {
        auto res = parse_property("Pr[<=1;7](<> true)", builder);
        REQUIRE(res == 0);
        auto expr = builder.getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == 7);  // number of runs
    }
    SUBCASE("Probability estimation query without runs")
    {
        auto res = parse_property("Pr[<=1](<> true)", builder);
        REQUIRE(res == 0);
        auto expr = builder.getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == -1);  // number of runs
    }
    SUBCASE("Value estimation query with 7 runs")
    {
        auto res = parse_property("E[<=1;7](max: 1)", builder);
        REQUIRE(res == 0);
        auto expr = builder.getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == 7);  // number of runs
    }
    SUBCASE("Value estimation query without runs")
    {
        auto res = parse_property("E[<=1](max: 1)", builder);
        REQUIRE(res == 0);
        auto expr = builder.getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == -1);  // number of runs
    }
}

TEST_CASE("Parsing implicit goals for learning queries")
{
    using Constants::Kind;
    auto doc = read_document("simpleSystem.xml");
    auto builder = QueryBuilder(doc);
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs[0].msg);
    const auto& warns = doc.get_warnings();
    if (not warns.empty()) {  // some warnings emitted at debug
        CHECK(warns.size() == 1);
        CHECK(warns[0].msg == "$Strict_invariant");
    }

    SUBCASE("Implicit time goal time priced")
    {
        auto res = parse_property("minPr[<=20]", builder);
        CHECK(res == -1);
        REQUIRE(errs.size() == 1);
    }

    SUBCASE("Implicit step goal time priced")
    {
        auto res = parse_property("minE(c)[#<=20]", builder);
        CHECK(res == -1);
        REQUIRE(errs.size() == 1);
    }

    SUBCASE("Implicit constraint goal expr priced")
    {
        auto res = parse_property("minE(c)[c<=25]", builder);
        CHECK(res == -1);
        REQUIRE(errs.size() == 1);
    }

    SUBCASE("Implicit time goal expr priced")
    {
        auto res = parse_property("minE(c)[<=20]", builder);
        CHECK(res == -1);
        REQUIRE(errs.size() == 1);
    }

    SUBCASE("Implicit step goal expr priced")
    {
        auto res = parse_property("minE(c)[#<=20]", builder);
        CHECK(res == -1);
        REQUIRE(errs.size() == 1);
    }

    SUBCASE("Explicit goal expr priced")
    {
        auto res = parse_property("minE(c)[<=20] :<> true", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        CHECK_MESSAGE(errs.empty(), errs[0].msg);
        if (not warns.empty()) {  // some warnings emitted at debug
            CHECK(warns.size() == 1);
            CHECK(warns[0].msg == "$Strict_invariant");
        }
    }

    SUBCASE("Explicit constraint goal expr priced")
    {
        auto res = parse_property("minE(c)[<=20] :<> c>=5", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        CHECK_MESSAGE(errs.empty(), errs[0].msg);
        if (not warns.empty()) {  // some warnings emitted at debug
            CHECK(warns.size() == 1);
            CHECK(warns[0].msg == "$Strict_invariant");
        }
    }
}

TEST_CASE("Function body is recovered after syntax error")
{
    auto f = document_fixture{};
    f.add_global_decl("void f(){ int x = }");

    auto doc = f.parse();
    CHECK(doc.get_globals().functions.size() == 1);
    CHECK(doc.get_globals().functions.back().body != nullptr);
}

TEST_CASE("Multiple functions despite early failure variable decl")
{
    auto f = document_fixture{};
    f.add_global_decl("void failing(){ int x = } void working(){ int x = 5;}");

    auto doc = f.parse();
    CHECK(doc.get_globals().functions.size() == 2);
    for (const Function& func : doc.get_globals().functions)
        CHECK(func.body != nullptr);
}

TEST_CASE("Multiple functions despite early failure type def")
{
    auto f = document_fixture{};
    f.add_global_decl("void failing(){ typedef int } void working(){ typedef int x;}");

    auto doc = f.parse();
    CHECK(doc.get_globals().functions.size() == 2);
    for (const Function& func : doc.get_globals().functions)
        CHECK(func.body != nullptr);
}

TEST_CASE("variable declaration failure shouldn't shadow declarations")
{
    auto f = document_fixture{};
    f.add_global_decl("int x = 0;int asdf\nint z = 0;");

    auto doc = f.parse();
    Symbol sym;
    CHECK(doc.get_globals().frame.resolve("x", sym));
    CHECK(doc.get_globals().frame.resolve("z", sym));
}

TEST_CASE("variable declaration failure shouldn't shadow declarations")
{
    auto f = document_fixture{};
    f.add_global_decl("typedef int x;\ntypedef int y\ntypedef int z;");

    auto doc = f.parse();
    Symbol sym;
    CHECK(doc.get_globals().frame.resolve("x", sym));
    CHECK(doc.get_globals().frame.resolve("z", sym));
}

TEST_CASE("variable declaration failure shouldn't shadow function")
{
    auto f = document_fixture{};
    f.add_global_decl("typedef int y\nvoid func(){}");

    auto doc = f.parse();
    CHECK(doc.get_globals().functions.size() == 1);
}

TEST_CASE("Missing closing curly brace for function")
{
    auto f = document_fixture{};
    f.add_global_decl("void func(){ ");

    auto doc = f.parse();
    CHECK(doc.get_globals().functions.size() == 1);
    CHECK(doc.get_errors().size() > 0);
}

TEST_CASE("Missing closing curly brace shouldn't shadow function")
{
    auto f = document_fixture{};
    f.add_global_decl("void func() {} void func2(){ ");

    auto doc = f.parse();
    CHECK(doc.get_globals().functions.size() == 2);
    CHECK(doc.get_errors().size() > 0);
}

TEST_CASE("Test leads to token is parsed correctly")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    CHECK_NOTHROW(f.parse_query("true --> true"));
}

TEST_CASE("Sim region cleanup causes memory errors (run with ASAN)")
{
    auto doc = read_document("lsc_example.xml");
    const auto& errs = doc.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto& templ = doc.get_templates().back();
    auto sims = templ.get_simregions();
    CHECK(sims.size() == 3);
}

TEST_CASE("Struct int,int initialization")
{
    auto doc =
        document_fixture{}.add_default_process().add_global_decl("const struct { int x; int y; } s = {1, 1};").parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Struct int,double initialization")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("const struct { int x; double y; } s = {1, 1.0};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Struct double,double initialization")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("const struct { double x; double y; } s = {1.0, 1};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Nested struct int,double initialization")
{
    auto doc =
        document_fixture{}
            .add_default_process()
            .add_global_decl("typedef struct { int x; double y; } S; struct { S s1; S s2; } s = {{5,5.5},{2,2.25}};")
            .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Meta struct")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("meta struct { int x; double y; } s = {1, 1.0};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Struct meta field")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("meta struct { int x; meta double y; } s = {1, 1.0};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Initializing doubles in struct with ints")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("meta struct { double x; meta double y; } s = {1, 1};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Initializing ints with double value")
{
    auto doc =
        document_fixture{}.add_default_process().add_global_decl("struct { int x; int y; } s = {1.1, 1.2};").parse();

    const auto& errs = doc.get_errors();
    REQUIRE(errs.size() == 2);
    CHECK(errs[0].msg == "$Invalid_initialiser");
    CHECK(errs[1].msg == "$Invalid_initialiser");
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Meta field in non meta struct")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("struct { int x; meta double y; } s = {1, 1.0};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Meta field in non meta struct")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("struct { int x; meta double y; } s = {1, 1.0};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Nested structs")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("struct { int x; struct { int y; double d;} data; } s = {1, {5, 5.0}};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Structs with arrays")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("struct { int x[2]; double y[2]; } s = {{1,1}, {5.0, 5.0}};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Array of structs")
{
    auto doc = document_fixture{}
                   .add_default_process()
                   .add_global_decl("struct { int x; double y;} s[2] = {{1,5.0}, {1,2.5}};")
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Pre increment precedence bug")
{
    auto doc = document_fixture{}
                   .add_global_decl("int i[2];")
                   .add_global_decl("void f(){ ++i[0]; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Post increment precedence bug")
{
    auto doc = document_fixture{}
                   .add_global_decl("int i[2];")
                   .add_global_decl("void f(){ i[0]++; }")
                   .add_default_process()
                   .parse();

    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Double post increment precedence")
{
    auto doc = document_fixture{}
                   .add_global_decl("int i = 0;")
                   .add_global_decl("void f(){ i++++; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    REQUIRE(errs.size() == 1);
    CHECK(errs[0].msg == "$Left_hand_side_value_expected");
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns[0].msg);
}

TEST_CASE("pre post increment precedence")
{
    auto doc = document_fixture{}
                   .add_global_decl("int i = 0;")
                   .add_global_decl("void f(){ ++i++; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    REQUIRE(errs.size() == 1);
    CHECK(errs[0].msg == "$Left_hand_side_value_expected");
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns[0].msg);
}

TEST_CASE("Double pre increment with forced precedence")
{
    auto doc = document_fixture{}
                   .add_global_decl("int i = 0;")
                   .add_global_decl("void f(){ (++i)++; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs[0].msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns[0].msg);
}

TEST_CASE("Double pre increment precedence")
{
    auto doc = document_fixture{}
                   .add_global_decl("int i = 0;")
                   .add_global_decl("void f(){ ++++i; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs[0].msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns[0].msg);
}

TEST_CASE("Increment with array subscripting and dot accessing")
{
    auto doc = document_fixture{}
                   .add_global_decl("struct { int x, y; } axy[2];")
                   .add_global_decl("void f(){ ++axy[0].x; axy[0].x++; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs[0].msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns[0].msg);
}

TEST_CASE("Increment with multiple array subscripting and dot accessing")
{
    auto doc = document_fixture{}
                   .add_global_decl("struct { int ai[2]; } aai[2];")
                   .add_global_decl("void f(){ ++aai[0].ai[0]; aai[0].ai[0]++; }")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs[0].msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns[0].msg);
}

TEST_CASE("Initializer: int")
{
    auto doc = document_fixture{}
                   .add_global_decl("typedef int[5,10] myint_t;")
                   .add_global_decl("myint_t my = 7;")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Initializer: array")
{
    auto doc = document_fixture{}
                   .add_global_decl("typedef int ia3_t[3];")
                   .add_global_decl("ia3_t ia = { 1, 2, 3 };")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_CASE("Initializer: struct")
{
    auto doc = document_fixture{}
                   .add_global_decl("typedef struct { int x, y; } xy_t;")
                   .add_global_decl("xy_t xy1 = { 1, 2 };")
                   .add_global_decl("xy_t xy2 = { x:1, y:2 };")
                   .add_default_process()
                   .parse();
    const auto& errs = doc.get_errors();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    const auto& warns = doc.get_warnings();
    CHECK_MESSAGE(warns.empty(), warns.front().msg);
}

TEST_SUITE_END();
