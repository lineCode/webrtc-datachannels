
# NOTE: INTERFACE library target does not directly create build output

add_library( boost_outcome INTERFACE )
target_include_directories( boost_outcome SYSTEM INTERFACE "boost.outcome/include" )
#target_compile_definitions(boost_outcome INTERFACE BOOST_OUTCOME_ENABLE_ADVANCED)
#target_compile_features(boost_outcome INTERFACE cxx_std_14)
get_target_property (BOOST_OUTCOME_IMPORTED_LOCATION boost_outcome INTERFACE_INCLUDE_DIRECTORIES)
message( "boost_outcome=${BOOST_OUTCOME_IMPORTED_LOCATION}" )

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json SYSTEM INTERFACE json/src)
#target_compile_features(nlohmann_json INTERFACE cxx_std_11)
get_target_property (nlohmann_json_IMPORTED_LOCATION nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
message( "nlohmann_json=${nlohmann_json_IMPORTED_LOCATION}" )

add_library(microsoft_gsl INTERFACE)
target_include_directories(microsoft_gsl SYSTEM INTERFACE GSL/include)
get_target_property (microsoft_gsl_IMPORTED_LOCATION microsoft_gsl INTERFACE_INCLUDE_DIRECTORIES)
message( "microsoft_gsl=${microsoft_gsl_IMPORTED_LOCATION}" )

add_library(Catch INTERFACE)
target_include_directories(Catch SYSTEM INTERFACE "Catch/single_include")
get_target_property (Catch_IMPORTED_LOCATION Catch INTERFACE_INCLUDE_DIRECTORIES)
message( "Catch=${Catch_IMPORTED_LOCATION}" )

add_library(FakeIt INTERFACE)
target_include_directories( FakeIt SYSTEM INTERFACE
  FakeIt/single_header
  FakeIt/include )
get_target_property (FakeIt_IMPORTED_LOCATION FakeIt INTERFACE_INCLUDE_DIRECTORIES)
message( "FakeIt=${FakeIt_IMPORTED_LOCATION}" )

add_library(better_enums INTERFACE)
target_include_directories(better_enums SYSTEM INTERFACE "better-enums")
#target_compile_features(better_enums INTERFACE cxx_std_11)
get_target_property (better_enums_IMPORTED_LOCATION better_enums INTERFACE_INCLUDE_DIRECTORIES)
message( "better_enums=${better_enums_IMPORTED_LOCATION}" )

add_library(boost_beast INTERFACE)
target_include_directories( boost_beast SYSTEM INTERFACE
  boost.beast/include )
get_target_property (BOOST_BEAST_IMPORTED_LOCATION boost_beast INTERFACE_INCLUDE_DIRECTORIES)
message( "Boost.beast=${BOOST_BEAST_IMPORTED_LOCATION}" )

#add_library(sol3 INTERFACE)
#target_include_directories( sol3 SYSTEM INTERFACE
#  boost.beast/include )
#get_target_property (SOL3_IMPORTED_LOCATION sol3 INTERFACE_INCLUDE_DIRECTORIES)
#message( "sol3=${SOL3_IMPORTED_LOCATION}" )

#add_subdirectory( sol2/single )

# required by folly
add_subdirectory( double-conversion )

# required by abseil
add_subdirectory( gtest )

# required by folly
add_subdirectory( glog )

# Use the `-DABSL_RUN_TESTS=ON` flag to run Abseil tests.  Note that if the `-DBUILD_TESTING=OFF` flag is passed then Abseil tests will not be run.
#set(ABSL_RUN_TESTS OFF)
SET(ABSL_RUN_TESTS OFF CACHE BOOL "ABSL_RUN_TESTS")
#set(BUILD_TESTING OFF)
add_subdirectory( abseil )
