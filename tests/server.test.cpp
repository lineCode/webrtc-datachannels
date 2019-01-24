/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "../src/algorithm/StringUtils.hpp" // TODO
#include <catch2/catch.hpp>

SCENARIO("strings can be left-padded") {
  GIVEN("a string") {
    std::string string = "1234";
    REQUIRE(string.length() == 4);
  }
  GIVEN("another string") {
    std::string string = "1234";
    REQUIRE(string.length() == 0);
  }
}
