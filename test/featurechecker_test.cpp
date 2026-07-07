// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020-2022 Aalborg University.

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

#include "document_fixture.h"

#include "utap/FeatureChecker.hpp"
#include "utap/utap.hpp"

#include <doctest/doctest.h>

TEST_CASE("Simple system")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("simpleSystem.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Simple SMC system")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("simpleSMCSystem.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(!checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Simple handshake system")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("simpleHandshakeSystem.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(!checker.get_supported_methods().stochastic);
}

TEST_CASE("Simply dynamic system")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("dynamic.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(!checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Clock rate")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("clock_rate2.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(!checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Clock rate expression")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("rate_expression.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Hybrid clock rate")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("rate_expression_hybrid.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Limited range clock rate")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("legal_symbolic_rates.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Integer Invariant")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("int_invariant.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Hybrid clock update")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("update_hybrid_clock.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Hybrid and normal clock update")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("update_hybrid_and_normal_clock.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(!checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Double comparison")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("double_compare.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(!checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Empty document")
{
    auto f = document_fixture{};
    auto doc = f.add_default_process().parse();
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Clock floating point initializer")
{
    auto f = document_fixture{};
    f.add_global_decl("clock c = 2.5;");
    auto doc = f.add_default_process().parse();
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(!checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Clock integer initializer")
{
    auto f = document_fixture{};
    f.add_global_decl("clock c = 2;");
    auto doc = f.add_default_process().parse();
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Clock uninitialized")
{
    auto f = document_fixture{};
    f.add_global_decl("clock c;");
    auto doc = f.add_default_process().parse();
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(checker.get_supported_methods().stochastic);
}

TEST_CASE("Channel priorities disable concrete simulator and smc")
{
    auto doc = UTAP::Document();
    parse_XML_buffer(read_content("channel_priorities.xml").c_str(), doc, true);
    auto checker = UTAP::FeatureChecker{doc};
    CHECK(checker.get_supported_methods().symbolic);
    CHECK(!checker.get_supported_methods().stochastic);
    CHECK(!checker.get_supported_methods().concrete);
}