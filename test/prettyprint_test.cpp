#include "document_fixture.h"

#include "utap/StatementBuilder.hpp"
#include "utap/expression.hpp"
#include "utap/utap.hpp"

#include <doctest/doctest.h>

#include <iostream>

using namespace UTAP;

TEST_SUITE_BEGIN("Pretty printing");

TEST_CASE("Symbolic queries")
{
    auto doc = read_document("simpleSystem.xml");
    const auto& errs = doc.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    auto builder = std::make_unique<QueryBuilder>(doc);
    auto& pb = *builder;
    auto os = std::stringstream{};
    // the redundant white-space is there to stress the parser
    SUBCASE("Reachability")
    {
        auto res = parse_property("E<> \t Process.L3", pb);
        REQUIRE(res == 0);
        pb.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        pb.getQuery().print(os);
        CHECK(os.str() == "E<> Process.L3");
    }
    SUBCASE("Safety")
    {
        auto res = parse_property("A[] \t Process.L3", pb);
        REQUIRE(res == 0);
        pb.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        pb.getQuery().print(os);
        CHECK(os.str() == "A[] Process.L3");
    }
    SUBCASE("Liveness")
    {
        auto res = parse_property("Process.L2 \t --> \t Process.L3", pb);
        REQUIRE(res == 0);
        pb.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        pb.getQuery().print(os);
        CHECK(os.str() == "Process.L2 --> Process.L3");
    }
    SUBCASE("Infimum")
    {
        auto res = parse_property("inf {  Process.L2  }  :  c", pb);
        REQUIRE(res == 0);
        pb.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        pb.getQuery().print(os);
        CHECK(os.str() == "inf{Process.L2}: c");
    }
    SUBCASE("Supremum")
    {
        auto res = parse_property("sup {  Process.L2  }  :  c", pb);
        REQUIRE(res == 0);
        pb.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        pb.getQuery().print(os);
        CHECK(os.str() == "sup{Process.L2}: c");
    }
    SUBCASE("Bounds")
    {
        auto res = parse_property("bounds {  Process.L2  }  :  c", pb);
        REQUIRE(res == 0);
        pb.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        pb.getQuery().print(os);
        CHECK(os.str() == "bounds{Process.L2}: c");
    }
}

TEST_CASE("Minimization pretty printing")
{
    auto doc = read_document("simpleSystem.xml");
    const auto& errs = doc.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    auto builder = QueryBuilder(doc);
    auto os = std::ostringstream{};
    SUBCASE("global time bound")
    {
        auto res = parse_property("minE(c)[<=20] {} -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[<=20] {} -> {} : <> c >= 20");
    }
    SUBCASE("step bound")
    {
        auto res = parse_property("minE(c)[#<=20] {} -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[#<=20] {} -> {} : <> c >= 20");
    }
    SUBCASE("cost bound without features")
    {
        auto res = parse_property("minE(c)[c<=20] : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[c<=20] : <> c >= 20");
    }
    SUBCASE("empty features")
    {
        auto res = parse_property("minE(c)[c<=20] {} -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[c<=20] {} -> {} : <> c >= 20");
    }
    SUBCASE("one discrete feature")
    {
        auto res = parse_property("minE(c)[c<=20] { Process.location } -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[c<=20] {Process.location} -> {} : <> c >= 20");
    }
    SUBCASE("one continuous feature")
    {
        auto res = parse_property("minE(c)[c<=20] {} -> { c } : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[c<=20] {} -> {c} : <> c >= 20");
    }
    SUBCASE("discrete and continuous features")
    {
        auto res = parse_property("minE(c)[c<=20] { Process.location } -> { c } : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "minE(c)[c<=20] {Process.location} -> {c} : <> c >= 20");
    }
}

TEST_CASE("Maximization pretty printing")
{
    auto doc = read_document("simpleSystem.xml");
    const auto& errs = doc.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    auto builder = QueryBuilder(doc);
    auto os = std::ostringstream{};
    SUBCASE("global time bound")
    {
        auto res = parse_property("maxE(c)[<=20] {} -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[<=20] {} -> {} : <> c >= 20");
    }
    SUBCASE("step bound")
    {
        auto res = parse_property("maxE(c)[#<=20] {} -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[#<=20] {} -> {} : <> c >= 20");
    }
    SUBCASE("cost bound without features")
    {
        auto res = parse_property("maxE(c)[c<=20] : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[c<=20] : <> c >= 20");
    }
    SUBCASE("empty features")
    {
        auto res = parse_property("maxE(c)[c<=20] {} -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[c<=20] {} -> {} : <> c >= 20");
    }
    SUBCASE("one discrete feature")
    {
        auto res = parse_property("maxE(c)[c<=20] { Process.location } -> {} : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[c<=20] {Process.location} -> {} : <> c >= 20");
    }
    SUBCASE("one continuous feature")
    {
        auto res = parse_property("maxE(c)[c<=20] {} -> { c } : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[c<=20] {} -> {c} : <> c >= 20");
    }
    SUBCASE("discrete and continuous features")
    {
        auto res = parse_property("maxE(c)[c<=20] { Process.location } -> { c } : <> c >= 20", builder);
        REQUIRE(res == 0);
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.typecheck();
        REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
        builder.getQuery().print(os);
        CHECK(os.str() == "maxE(c)[c<=20] {Process.location} -> {c} : <> c >= 20");
    }
}

TEST_CASE("saveStrategy pretty printing")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    const auto& strat = f.parse_query("strategy Name = control: A[] true");
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    auto res = f.parse_query("saveStrategy(\"path\", Name)");
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(res.subjections.at(0) == &strat);
    CHECK(res.intermediate.str() == "saveStrategy(\"path\", Name)");
}

TEST_CASE("Probability compare pretty print")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query = f.parse_query("Pr[<=20] (<> true) >= Pr[<=5]([] false)").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    REQUIRE(query.get_kind() == UTAP::Constants::PROBA_CMP);
    CHECK(query.str() == "Pr[<=20] (<> true) >= Pr[<=5] ([] false)");
}

TEST_CASE("Simulate pretty prints")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query1 = f.parse_query("simulate[<=20;1000] {5, true} : 100 : true").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    REQUIRE(query1.get_kind() == UTAP::Constants::SIMULATEREACH);
    CHECK(query1.str() == "simulate[<=20; 1000] {5, true} : 100 : true");

    auto query2 = f.parse_query("simulate[#<=10;500] {25, false}").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    REQUIRE(query2.get_kind() == UTAP::Constants::SIMULATE);
    CHECK(query2.str() == "simulate[#<=10; 500] {25, false}");
}

TEST_CASE("loadStrategy pretty printing")
{
    auto doc = read_document("simpleSystem.xml");
    const auto& errs = doc.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    auto builder = QueryBuilder{doc};
    auto res = parse_property("strategy Name = loadStrategy{Process.location}->{c}(\"path\")", builder);
    CHECK(res == 0);
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    builder.typecheck();
    CHECK_MESSAGE(errs.empty(), errs.front().msg);
    auto os = std::ostringstream{};
    builder.getQuery().print(os);
    CHECK(os.str() == "loadStrategy{Process.location} -> {c}(\"path\")");
}

TEST_CASE("Array access pretty printing")
{
    auto f = document_fixture{}.add_global_decl("int arr[5];").add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query1 = f.parse_query("E<> arr[2] == 5").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(query1.str() == "E<> arr[2] == 5");
}

TEST_CASE("Chaining conjunctions")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query1 = f.parse_query("E<> true && true && true").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(query1.str() == "E<> true && true && true");
}

TEST_CASE("Chaining disjunctive conjunctions")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query1 = f.parse_query("E<> true && true || true && true").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(query1.str() == "E<> true && true || true && true");
}

TEST_CASE("Chaining disjunctive conjunctions")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query1 = f.parse_query("E<> true || true && true || true").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(query1.str() == "E<> true || true && true || true");
}

TEST_CASE("Chaining disjunctive conjunctions with outer conjunction")
{
    auto f = document_fixture{}.add_default_process().build_query_fixture();
    const auto& errs = f.get_errors();
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);

    auto query1 = f.parse_query("E<> (true || true && true || true) && false").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(query1.str() == "E<> (true || true && true || true) && false");

    auto query2 = f.parse_query("E<> false && (true || true && true || true)").intermediate;
    REQUIRE_MESSAGE(errs.empty(), errs.front().msg);
    CHECK(query2.str() == "E<> false && (true || true && true || true)");
}

TEST_CASE("Post incrementing an identifier should not require parenthesis")
{
    auto frame = Frame::make();
    auto test_symbol = frame.add_symbol("foo", Type::create_primitive(Constants::INT), {});
    auto id = Expression::create_identifier(test_symbol);
    auto expr = Expression::create_unary(Constants::POST_INCREMENT, id);

    CHECK(expr.str() == "foo++");
}

TEST_SUITE_END();
