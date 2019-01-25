
cmake_minimum_required( VERSION 3.13.3 FATAL_ERROR )

set( PROJECT_NAME "${ROOT_PROJECT_NAME}-tests" )

# Get CMAKE_MODULE_PATH from parent project
list(APPEND CMAKE_MODULE_PATH "${${ROOT_PROJECT_NAME}_CMAKE_MODULE_PATH}")

if (ENABLE_CODE_COVERAGE)
  set( CATCH2_TEST_SETTINGS -r junit -o unit_test_result.xml )
else()
  set( CATCH2_TEST_SETTINGS -r console --use-colour yes --order rand --durations yes --warn NoTests --invisibles)
endif()

include( testRunner ) # start tests as CMake targets

find_package( Sanitizers ) # see https://github.com/arsenm/sanitizers-cmake
# sanitizer_add_blacklist_file("blacklist.txt") # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support

macro(add_to_tests_list source_list)
  list(APPEND UNIT_TEST_SOURCE_LIST ${source_list})
endmacro()

macro(set_test_compile_options target)
  target_compile_options(${target} PRIVATE
              $<$<CXX_COMPILER_ID:MSVC>:
              /W3 # Set warning level
              #/WX # Treats all compiler warnings as errors.
              >
              $<$<CXX_COMPILER_ID:GNU>:
              -Wformat=2
              -Wall
              -W
              "$<$<CONFIG:RELEASE>:-Werror>" # Treats all compiler warnings as errors.
              -Wpedantic
              #-fsanitize-address-use-after-scope # https://clang.llvm.org/docs/AddressSanitizer.html
              #-fsanitize=pointer-compare # https://kristerw.blogspot.com/2018/06/useful-gcc-address-sanitizer-checks-not.html
              -fno-omit-frame-pointer # https://github.com/google/sanitizers/wiki/AddressSanitizer#using-addresssanitizer
              >
              $<$<CXX_COMPILER_ID:Clang>:
              -Wformat=2
              -Wall
              -W
              "$<$<CONFIG:RELEASE>:-Werror>" # Treats all compiler warnings as errors.
              -Wpedantic
              -Wdeprecated-register
              #-fsanitize-address-use-after-scope # https://clang.llvm.org/docs/AddressSanitizer.html
              #-fsanitize=pointer-compare # https://kristerw.blogspot.com/2018/06/useful-gcc-address-sanitizer-checks-not.html
              -fno-omit-frame-pointer # https://github.com/google/sanitizers/wiki/AddressSanitizer#using-addresssanitizer
              >
          )

  # @see https://github.com/eranpeer/FakeIt
  target_compile_options( ${target} PRIVATE
    # If you don't use /ZI, you will have exceptions mocking destructors (which includes unique_ptr and other smart pointers).
    $<$<CXX_COMPILER_ID:MSVC>:/ZI>
    # On GCC, optimization flag O2 and O3 are not supported. You must compile the test project with -O1 or -O0.
    $<$<CXX_COMPILER_ID:GNU>:-O0>
    $<$<CXX_COMPILER_ID:Clang>:-O0> )

  set_target_properties( ${target} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CMAKE_CXX_STANDARD_REQUIRED ON
    CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/tests )
endmacro()

macro(tests_add_executable target source_list)
  list(APPEND UNIT_TEST_TARGETS ${target})

  add_to_tests_list(${source_list})

  add_executable(${target} ${source_list})

  add_sanitizers( ${target} ) # https://github.com/arsenm/sanitizers-cmake#build-targets-with-sanitizer-support
  message( "added sanitizers to ${target}" )

  target_link_libraries(${target} PRIVATE test_main)

  set_test_compile_options( ${target} )

  add_test(
    NAME ${target}
    COMMAND ${target} ${CATCH2_TEST_SETTINGS})

  add_test_command_targets( ${target} )

  if(AUTORUN_TESTS)
    add_test_autostarter( ${target} )
  endif()
endmacro()

add_library( test_main OBJECT
  main.cpp
  ${${ROOT_PROJECT_NAME}_SRCS} # all source files  of root project without main.cpp
  ${THIRDPARTY_SOURCES}
  testsCommon.h # include in IDE
  )

# ensure that dependencies build before <target> does.
add_dependencies(test_main ${ROOT_PROJECT_NAME})

set_test_compile_options( test_main )

target_link_libraries(test_main PUBLIC
  # 3dparty libs
  ${USED_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
)

target_link_libraries( test_main PUBLIC
  Catch FakeIt nlohmann_json microsoft_gsl boost_outcome better_enums )

target_compile_definitions( test_main PUBLIC
  ${WEBRTC_DEFINITIONS} ${RAPIDJSON_DEFINITIONS} )

target_link_directories( test_main PUBLIC ${WEBRTC_LIB_PATHS} )

target_include_directories( test_main SYSTEM PUBLIC
  ${ROOT_PROJECT_DIR}/src
  ${WEBRTC_INCLUDES}
  ${THIRDPARTY_FILES}
  ${ABSEIL_BASE_IMPORTED_LOCATION})

option(AUTORUN_TESTS "Autorun tests" ON) # see add_test_autostarter

option(BUILD_TESTS "Enable tests" ON)
if(BUILD_TESTS)
  message( "${PROJECT_NAME} testing enabled" )

  set ( server_deps
    server.test.cpp
)
  tests_add_executable(server "${server_deps}")

  set ( utils_deps
    utils.test.cpp
)
  tests_add_executable(utils "${utils_deps}")
endif()

#add_to_tests_list(utils)
#add_to_tests_list(server)

tests_add_executable(check_all ${UNIT_TEST_SOURCE_LIST})
#message(FATAL_ERROR ${UNIT_TEST_SOURCE_LIST})

# "server" is one of test program names
add_custom_command( TARGET server POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_SOURCE_DIR}/tests/data ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/tests/data )

enable_testing()

# Run ALL tests with verbose output
# Usage: cmake --build build --target run_all_tests
add_custom_target(run_all_tests
    DEPENDS ${ROOT_PROJECT_NAME} ${ALL_TESTS_LIST_VERBOSE}
)
foreach(target IN LISTS ALL_TESTS_LIST_VERBOSE)
  add_dependencies(run_all_tests ${target})
endforeach()

option(ENABLE_CODE_COVERAGE "ENABLE CODE COVERAGE" OFF)
if (ENABLE_CODE_COVERAGE)
  set(LCOV_REMOVE_EXTRA "'*/src/include/webrtc/*'" "'${ROOT_PROJECT_DIR}/lib/*'" "'${ROOT_PROJECT_DIR}/submodules/*'" "'test/*'" "'ext/*'" "'abseil/*'" "'submodules/*'" "'*/tests/*'" "'scripts/*'" "'lib/*'" "'build/*'" "'bin/*'")
  set(CMAKE_BUILD_TYPE Coverage)
  # test coverage build configuration for C++: debug build, no optimization, profiling enabled
  set(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
  # include the cmake code coverage module
  include( CodeCoverage )
  # cmake code coverage module target
  setup_target_for_coverage(
    # Name for custom target.
    unit_tests_coverage
    # Name of the test driver executable that runs the tests.
    # NOTE! This should always have a ZERO as exit code
    # otherwise the coverage generation will not complete.
    check_all
    coverage # Name of output directory.
    )
  message("added make code coverage for ${PROJECT_NAME}_coverage")
else()
  message(WARNING "code coverage off")
endif()

#find_package(Sanitizers)
#if(HAS_Address_SANITIZER AND HAS_Fuzzer_SANITIZER)
#	add_executable(fuzz fuzzing.cpp)
#	target_link_libraries(fuzz PUBLIC gltfpp Sanitizer::Fuzzer Sanitizer::Address)
#endif()
