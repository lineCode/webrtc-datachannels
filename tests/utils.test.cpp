/*
 * Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include "algo/DispatchQueue.hpp"
#include "algo/NetworkOperation.hpp"
#include "storage/path.hpp"
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "testsCommon.h"

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

struct SomeInterface {
  virtual int foo(int) = 0;
  virtual int bar(std::string) = 0;
};

SCENARIO("utils", "[network]") {

  using namespace gloer::storage;
  using namespace gloer::algo;

  GIVEN("mockExample") {
    // Instantiate a mock object.
    Mock<SomeInterface> mock;
    // mock.get().bar("");

    // Setup mock behavior.
    When(Method(mock, foo)).Return(1); // Method mock.foo will return 1 once.

    // Fetch the mock instance.
    SomeInterface& i = mock.get();

    // Will print "1".
    REQUIRE(i.foo(0) == 1);

    Mock<DispatchQueue> fakeDispatchQueue;

    When(Method(fakeDispatchQueue, sizeGuess)).Return(0);

    REQUIRE(fakeDispatchQueue.get().sizeGuess() == 0);
  }

  GIVEN("getFileContentsFromWorkdir") {

    const ::fs::path workdir = gloer::storage::getThisBinaryDirectoryPath();
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
