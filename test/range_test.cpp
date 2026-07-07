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

#include "utap/range.hpp"

#include <doctest/doctest.h>

#include <vector>

using UTAP::range_t;

TEST_CASE("range_t")
{
    constexpr auto d_eps = std::numeric_limits<double>::epsilon();
    constexpr auto d_inf = std::numeric_limits<double>::infinity();
    constexpr auto d_ninf = -d_inf;
    constexpr auto d_max = std::numeric_limits<double>::max();
    constexpr auto d_min = std::numeric_limits<double>::lowest();

    constexpr auto f_eps = std::numeric_limits<float>::epsilon();
    constexpr auto f_inf = std::numeric_limits<float>::infinity();
    constexpr auto f_ninf = -f_inf;
    constexpr auto f_min = std::numeric_limits<float>::lowest();

    constexpr auto i_max = std::numeric_limits<int>::max();
    constexpr auto i_min = std::numeric_limits<int>::min();

    SUBCASE("Next after 1")
    {
        // Tests the definition of epsilon:
        auto d = 1.0;
        auto next_d = UTAP::next_value(d);
        CHECK(next_d == 1.0 + d_eps);
    }

    SUBCASE("Next after 1f")
    {
        // Tests the definition of epsilon:
        auto f = 1.0f;
        auto next_f = UTAP::next_value(f);
        CHECK(next_f == 1.0f + f_eps);
    }

    SUBCASE("Next after max")
    {
        // Tests that all values converge at the infinity
        auto d = std::numeric_limits<double>::max();
        auto next_d = UTAP::next_value(d);
        CHECK(next_d == d_inf);
    }

    SUBCASE("Next after max float")
    {
        auto f = std::numeric_limits<float>::max();
        auto next_f = UTAP::next_value(f);
        CHECK(next_f == f_inf);
    }

    SUBCASE("Previous before lowest")
    {
        auto d = d_min;
        auto prev_d = UTAP::prev_value(d);
        CHECK(prev_d == d_ninf);
    }

    SUBCASE("Previous before lowest float")
    {
        auto f = f_min;
        auto prev_f = UTAP::prev_value(f);
        CHECK(prev_f == f_ninf);
    }

    SUBCASE("Next after max-1")
    {
        // Tests that all values converge at the infinity
        auto i = i_max - 1;
        auto next_i = UTAP::next_value(i);
        CHECK(next_i == i_max);
    }

    SUBCASE("Previous before min int")
    {
        auto i = i_min + 1;
        auto prev_i = UTAP::prev_value(i);
        CHECK(prev_i == i_min);
    }

    SUBCASE("Next after double negative infinity")
    {
        auto d = -d_inf;
        auto next_d = UTAP::next_value(d);
        CHECK(next_d == d_min);
    }

    SUBCASE("Previous before infinity double")
    {
        auto d = d_inf;
        auto prev_d = UTAP::prev_value(d);
        CHECK(prev_d == d_max);
    }

    SUBCASE("Greater than infinity")
    {
        auto r1 = range_t<double>(0, d_inf);
        r1 = r1.gt(d_inf);
        CHECK(r1.empty());
    }

    SUBCASE("Less than negative double infinity")
    {
        auto r1 = range_t<double>{d_ninf, 0};
        r1 = r1.lt(d_ninf);
        CHECK(r1.empty());
    }
    SUBCASE("constexpr")
    {
        constexpr auto r1 = range_t<double>{1, 10};
        static_assert(r1.first() == 1);
        static_assert(r1.last() == 10);
        static_assert(!r1.contains(0));
        static_assert(r1.contains(5));
        static_assert(!r1.contains(20));
        constexpr auto r2 = range_t<double>{2, 3};
        constexpr auto r3 = r1 + r2;
        static_assert(r3.first() == 3);
        static_assert(r3.last() == 13);
        constexpr auto r4 = r1 - r2;
        static_assert(r4.first() == -2);
        static_assert(r4.last() == 8);
        constexpr auto r5 = r1 * r2;
        static_assert(r5.first() == 2);
        static_assert(r5.last() == 30);
        static_assert(!(r1 < r2));
        static_assert(!(r1 > r2));
    }
}

TEST_CASE("range_t container")
{
    auto ranges = std::vector<range_t<int>>{};
    ranges.emplace_back(1, 2);
    REQUIRE(ranges.size() == 1);
    ranges.emplace_back(3, 4);
    REQUIRE(ranges.size() == 2);
    ranges.emplace(std::begin(ranges), ranges.back());
    REQUIRE(ranges.size() == 3);
    CHECK(ranges[0] == ranges[2]);
}