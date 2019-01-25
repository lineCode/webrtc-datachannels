/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "../src/algo/StringUtils.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "testsCommon.h"

SCENARIO("matchers", "[matchers]") {
  REQUIRE_THAT("Hello olleH",
               Predicate<std::string>(
                   [](std::string const& str) -> bool { return str.front() == str.back(); },
                   "First and last character should be equal"));
}

SCENARIO("vectors can be sized and resized", "[vector]") {

  GIVEN("A vector with some items") {
    std::vector<int> v(5);

    REQUIRE(v.size() == 5);
    REQUIRE(v.capacity() >= 5);

    WHEN("the size is increased") {
      v.resize(10);

      THEN("the size and capacity change") {
        REQUIRE(v.size() == 10);
        REQUIRE(v.capacity() >= 10);
      }
    }
    WHEN("the size is reduced") {
      v.resize(0);

      THEN("the size changes but not capacity") {
        REQUIRE(v.size() == 0);
        REQUIRE(v.capacity() >= 5);
      }
    }
    WHEN("more capacity is reserved") {
      v.reserve(10);

      THEN("the capacity changes but not the size") {
        REQUIRE(v.size() == 5);
        REQUIRE(v.capacity() >= 10);
      }
    }
    WHEN("less capacity is reserved") {
      v.reserve(0);

      THEN("neither size nor capacity are changed") {
        REQUIRE(v.size() == 5);
        REQUIRE(v.capacity() >= 5);
      }
    }
  }
}

SCENARIO("stringUtils", "[algo]") {
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
