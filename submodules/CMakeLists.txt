
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

add_library(better_enums INTERFACE)
target_include_directories(better_enums SYSTEM INTERFACE "better-enums")
#target_compile_features(better_enums INTERFACE cxx_std_11)
get_target_property (better_enums_IMPORTED_LOCATION better_enums INTERFACE_INCLUDE_DIRECTORIES)
message( "better_enums=${better_enums_IMPORTED_LOCATION}" )
