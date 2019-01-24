/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "../src/algo/DispatchQueue.hpp"
#include "../src/algo/NetworkOperation.hpp"
#include "../src/storage/path.hpp"
#include <catch2/catch.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem; // from <filesystem>

SCENARIO("utils") {
  using namespace gloer::storage;
  using namespace gloer::algo;

  GIVEN("getFileContentsFromWorkdir") {
    const fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
    REQUIRE(getFileContents(workdir / "data/asset_complete.json").length() == 264);
  }

  GIVEN("DispatchQueue") {
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

  GIVEN("NetworkOperation") {
    WS_OPCODE WS_OPCODE_CANDIDATE = WS_OPCODE::CANDIDATE;
    REQUIRE(Opcodes::opcodeToStr(WS_OPCODE_CANDIDATE) == "1");
    REQUIRE(Opcodes::opcodeToDescrStr(WS_OPCODE_CANDIDATE) == "CANDIDATE");
    REQUIRE(Opcodes::wsOpcodeFromStr("1")._to_integral() == 1);
    REQUIRE(WS_OPCODE_CANDIDATE._name() == Opcodes::wsOpcodeFromStr("1")._name());
    REQUIRE(Opcodes::wsOpcodeFromDescrStr("CANDIDATE")._to_integral() == 1);
  }
}
