/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "../src/algorithm/DispatchQueue.hpp"
#include "../src/filesystem/path.hpp"
#include <catch2/catch.hpp>
#include <memory>

#include "absl/strings/str_join.h"
#include <iostream>
#include <string>
#include <vector>

SCENARIO("utils") {
  using namespace utils::filesystem;
  using namespace utils::algo;

  GIVEN("getFileContents") { REQUIRE(getFileContents("data/asset_complete.json").length() == 264); }

  GIVEN("DispatchQueue") {
    std::vector<std::string> v = {"foo", "bar", "baz"};
    std::string s = absl::StrJoin(v, "-");
    REQUIRE(s == "foo-bar-baz");

    std::shared_ptr<DispatchQueue> testQueue =
        std::make_shared<DispatchQueue>(std::string{"WebSockets Server Dispatch Queue"}, 0);
    std::string dispatchResult = "0";
    testQueue->dispatch([&dispatchResult] { dispatchResult = "1"; });
    REQUIRE(dispatchResult == "0");
    testQueue->DispatchQueued();
    REQUIRE(dispatchResult == "1");
    testQueue->DispatchQueued();
    REQUIRE(dispatchResult == "1");
    testQueue->dispatch([&dispatchResult] { dispatchResult = "2"; });
    testQueue->dispatch([&dispatchResult] { dispatchResult = "3"; });
    REQUIRE(dispatchResult == "1");
    testQueue->DispatchQueued();
    REQUIRE(dispatchResult == "3");
  }
}
