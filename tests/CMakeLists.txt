
cmake_minimum_required( VERSION 3.13.2 FATAL_ERROR )

set( PROJECT_NAME "gloer-tests" )
set( ROOT_PROJECT_DIR ".." )
set( CATCH2_TEST_SETTINGS -r console --use-colour yes --order rand --durations yes --warn NoTests --invisibles)

include( ../cmake/Utils.cmake )
include( ../Dependencies.cmake )
add_library( absl INTERFACE IMPORTED)

#include_directories (
#    ${TEST_SOURCE_DIR}/src # https://cmake.org/cmake/help/v3.6/variable/PROJECT-NAME_SOURCE_DIR.html
#    ${TEST_SOURCE_DIR}
#)

include(testRunner)

macro(add_to_tests_list test_file_name)
  list(APPEND UNIT_TEST_SOURCE_LIST ${test_file_name}.test.cpp)
endmacro()

set(WEBRTC_INCLUDES ${WEBRTC_SRC_PATH}/include
  ${WEBRTC_SRC_PATH}/include/webrtc
  #${WEBRTC_SRC_PATH}/third_party/abseil-cpp # we use custom abseil version
  ${WEBRTC_SRC_PATH}/third_party/jsoncpp/source/include
  ${WEBRTC_SRC_PATH}/third_party/libyuv/include)

set(THIRDPARTY_FILES
  "../lib/"
  "../lib/whereami/"
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LUA_INCLUDE_DIR}
  ${RANG_INCLUDE_DIR}
  ${G3LOG_INCLUDE_DIR}
  ${WEBRTC_INCLUDES}
  ${FOLLY_INCLUDE_DIR}
  ${CLANG_PATH}/include)

macro(tests_add_executable target source_list)
  list(APPEND UNIT_TEST_TARGETS ${target})

  add_executable(${target} ${source_list})

  target_link_libraries(${target} PRIVATE test_main)

  target_include_directories( ${target} SYSTEM PUBLIC
    ${ABSEIL_BASE_IMPORTED_LOCATION} )

  target_include_directories( ${target} SYSTEM PRIVATE
    ${ROOT_PROJECT_DIR}/src
    ${WEBRTC_INCLUDES}
    ${THIRDPARTY_FILES})

  target_compile_options(${target} PRIVATE
              $<$<CXX_COMPILER_ID:MSVC>:/W3 /WX>
              $<$<CXX_COMPILER_ID:GNU>:-Wall -W -Werror -Wpedantic>
              $<$<CXX_COMPILER_ID:Clang>:-Wall -W -Werror -Wpedantic>
          )

  add_test(
    NAME ${target}
    COMMAND ${target} ${CATCH2_TEST_SETTINGS})

  set_target_properties( ${target} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    CMAKE_CXX_STANDARD_REQUIRED ON
    CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG> )

  target_include_directories( ${target} SYSTEM PRIVATE
    ${THIRDPARTY_FILES} )

  target_compile_definitions( ${target} PRIVATE
    ${WEBRTC_DEFINITIONS} ${RAPIDJSON_DEFINITIONS} )

  target_link_directories( ${target} PRIVATE ${WEBRTC_LIB_PATHS} )

  add_test_command_targets( ${target} )

  if(AUTORUN_TESTS)
    add_test_autostarter( ${target} )
  endif()
endmacro()

add_library(test_main OBJECT
  main.cpp
  ${ROOT_PROJECT_DIR}/src/filesystem/path.cpp
  ${ROOT_PROJECT_DIR}/src/log/Logger.cpp
  ../lib/whereami/whereami.c)

# ensure that dependencies build before <target> does.
add_dependencies(test_main gloer)

set_target_properties( test_main PROPERTIES
  # OUTPUT_NAME ${target}
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  #IMPORTED_LOCATION "/opt/Qt${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/gcc_64/lib"
  #INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/libbar"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG> )

target_link_libraries(test_main PUBLIC
  # 3dparty libs
  ${USED_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
)

target_link_libraries( test_main PUBLIC
  Catch nlohmann_json microsoft_gsl boost_outcome better_enums )

target_include_directories( test_main SYSTEM PRIVATE
  ${ROOT_PROJECT_DIR}/src
  ${WEBRTC_INCLUDES}
  ${THIRDPARTY_FILES})

#add_to_tests_list(utils)
#add_to_tests_list(server)

#tests_add_executable(${PROJECT_NAME} ${UNIT_TEST_SOURCE_LIST})

option(AUTORUN_TESTS "Autorun tests" ON)

option(BUILD_TESTS "Enable tests" ON)
if(BUILD_TESTS)
  message( "${PROJECT_NAME} testing enabled" )

  set ( server_deps
    server.test.cpp
    ${ROOT_PROJECT_DIR}/src/algorithm/StringUtils.cpp )
  tests_add_executable(server "${server_deps}")

  set ( utils_deps
    utils.test.cpp
    ${ROOT_PROJECT_DIR}/src/algorithm/DispatchQueue.cpp
    ${ROOT_PROJECT_DIR}/src/algorithm/NetworkOperation.cpp )
  tests_add_executable(utils "${utils_deps}")
endif()

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

enable_testing()

# Run ALL tests with verbose output
# Usage: cmake --build build --target run_all_tests
add_custom_target(run_all_tests
    DEPENDS gloer ${ALL_TESTS_LIST_VERBOSE}
)
foreach(target IN LISTS ALL_TESTS_LIST_VERBOSE)
  add_dependencies(run_all_tests ${target})
endforeach()

#find_package(Sanitizers)
#if(HAS_Address_SANITIZER AND HAS_Fuzzer_SANITIZER)
#	add_executable(fuzz fuzzing.cpp)
#	target_link_libraries(fuzz PUBLIC gltfpp Sanitizer::Fuzzer Sanitizer::Address)
#endif()
