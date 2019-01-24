/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "../src/algo/StringUtils.hpp"
#include <catch2/catch.hpp>

#include <iostream>
#include <string>
#include <vector>

SCENARIO("stringUtils") {
  using namespace gloer::algo;

  GIVEN("trim_whitespace") {

    REQUIRE(trim_whitespace(std::string(" \n 1 2 34")) == "1 2 34");
    REQUIRE(trim_whitespace(std::string("1 2 34 \n ")) == "1 2 34");
    REQUIRE(trim_whitespace(std::string(" \n 1234")) == "1234");
    REQUIRE(trim_whitespace(std::string("1234 ")) == "1234");
    REQUIRE(trim_whitespace(std::string("1234")) == "1234");
    REQUIRE(trim_whitespace(std::string(" 1234 ")) == "1234 ");
    REQUIRE(trim_whitespace(std::string(" 1234 \n")) == "1234 ");
    REQUIRE(trim_whitespace(std::string("1234 \t")) == "1234");
    REQUIRE(trim_whitespace(std::string("\t")) == "");
  }
}
