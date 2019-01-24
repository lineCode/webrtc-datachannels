
cmake_minimum_required( VERSION 3.13.2 FATAL_ERROR )

set( PROJECT_NAME "gloer-tests" )

set(UNIT_TEST_LIST
  server)

include_directories (
    ${TEST_SOURCE_DIR}/src # https://cmake.org/cmake/help/v3.6/variable/PROJECT-NAME_SOURCE_DIR.html
    ${TEST_SOURCE_DIR}
)
foreach(NAME IN LISTS UNIT_TEST_LIST)
  list(APPEND UNIT_TEST_SOURCE_LIST ${NAME}.test.cpp)
endforeach()

add_executable(${PROJECT_NAME}
  main.cpp
  ${UNIT_TEST_SOURCE_LIST})

target_link_libraries(${PROJECT_NAME}
  PUBLIC Catch)
get_target_property (Catch_IMPORTED_LOCATION Catch INTERFACE_INCLUDE_DIRECTORIES)
message( "Catch_IMPORTED_LOCATION=${Catch_IMPORTED_LOCATION}" )

#target_include_directories(${PROJECT_NAME} PRIVATE ${Catch_IMPORTED_LOCATION})

#target_link_libraries( ${PROJECT_TARGET_EXE} PRIVATE Catch )

#target_include_directories(${PROJECT_NAME}
#  PUBLIC ../externals/catch2/)

file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(
  NAME ${PROJECT_NAME}
  COMMAND ${PROJECT_NAME} -o report.xml -r junit)

option(${PROJECT_NAME}_BUILD_TESTS "Enable tests" ON)
if(${PROJECT_NAME}_BUILD_TESTS)
  message( "${PROJECT_NAME} testing enabled" )
  enable_testing()
endif()

add_custom_target(ctest-cleanup
    ${CMAKE_COMMAND} -E remove Tests/ctest.log
    ${CMAKE_COMMAND} -E remove Tests
    ${CMAKE_COMMAND} -E remove Testing
    ${CMAKE_COMMAND} -E remove test
)

## Force running test after build (after each change in source files)
#add_custom_command(TARGET MainTest ArgparseTest
#    POST_BUILD
#    COMMAND  ${CMAKE_COMMAND} --build . --target ${PROJECT_NAME}_check_verbose
#    DEPENDS ctest-cleanup
#)

#find_package(Sanitizers)
#if(HAS_Address_SANITIZER AND HAS_Fuzzer_SANITIZER)
#	add_executable(fuzz fuzzing.cpp)
#	target_link_libraries(fuzz PUBLIC gltfpp Sanitizer::Fuzzer Sanitizer::Address)
#endif()
