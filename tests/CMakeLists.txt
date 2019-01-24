
cmake_minimum_required( VERSION 3.13.2 FATAL_ERROR )

set( PROJECT_NAME "gloer-tests" )
set( ROOT_PROJECT_DIR ".." )

set( CATCH2_TEST_SETTINGS -r console --use-colour yes --order rand --durations yes --warn NoTests --invisibles)

include(testRunner)

macro(add_to_tests_list test_file_name)
  list(APPEND UNIT_TEST_SOURCE_LIST ${test_file_name}.test.cpp)
endmacro()

macro(tests_add_executable target source_list)
  list(APPEND UNIT_TEST_TARGETS ${target})

  add_executable(${target} ${source_list})

  target_link_libraries(${target} PRIVATE test_main)

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
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/tests )

  add_test_command_targets( ${target} )

  if(AUTORUN_TESTS)
    add_test_autostarter( ${target} )
  endif()
endmacro()

add_library(test_main OBJECT
  main.cpp
  ${ROOT_PROJECT_DIR}/src/filesystem/path.cpp
  ${ROOT_PROJECT_DIR}/src/log/Logger.cpp
  ${THIRDPARTY_SOURCES})

# ensure that dependencies build before <target> does.
add_dependencies(test_main gloer)

set_target_properties( test_main PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/tests )

target_link_libraries(test_main PUBLIC
  # 3dparty libs
  ${USED_3DPARTY_LIBS}
  # system libs
  ${USED_SYSTEM_LIBS}
)

target_link_libraries( test_main PUBLIC
  Catch nlohmann_json microsoft_gsl boost_outcome better_enums )

target_compile_definitions( test_main PUBLIC
  ${WEBRTC_DEFINITIONS} ${RAPIDJSON_DEFINITIONS} )

target_link_directories( test_main PUBLIC ${WEBRTC_LIB_PATHS} )

target_include_directories( test_main SYSTEM PUBLIC
  ${ROOT_PROJECT_DIR}/src
  ${WEBRTC_INCLUDES}
  ${THIRDPARTY_FILES}
  ${ABSEIL_BASE_IMPORTED_LOCATION})

#add_to_tests_list(utils)
#add_to_tests_list(server)

#tests_add_executable(${PROJECT_NAME} ${UNIT_TEST_SOURCE_LIST})

option(AUTORUN_TESTS "Autorun tests" ON) # see add_test_autostarter

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

#message(FATAL_ERROR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
# "server" is one of test program names
add_custom_command( TARGET server POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_SOURCE_DIR}/tests/data ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/tests/data )
#file(COPY data DESTINATION ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)

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
