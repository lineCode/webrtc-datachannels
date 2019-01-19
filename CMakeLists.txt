
cmake_minimum_required(VERSION 3.13.2 FATAL_ERROR)

set(PROJECT_NAME "example-server")
set(PROJECT_TARGET_EXE server_target)

project(${PROJECT_NAME})

# TODO
# option(BUILD_DOCS "Build documentation using Doxygen" ON)
# TODO
# option(BUILD_TESTS "Build tests" ON)

### Extra functionality
include(cmake/Utils.cmake)

set_project_version(1 0 0) # from Utils.cmake

set_cmake_module_paths("${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules") # from Utils.cmake

print_cmake_system_info() # from Utils.cmake

check_supported_os() # from Utils.cmake

# Qt relies on some bundled tools for code generation, such as moc for meta-object code generation, uic for widget layout and population, and rcc for virtual filesystem content generation.
# The AUTOMOC target property controls whether cmake(1) inspects the C++ files in the target to determine if they require moc to be run, and to create rules to execute moc at the appropriate time. If a Q_OBJECT or Q_GADGET macro is found in a header file, moc will be run on the file.
set(CMAKE_AUTOMOC ON)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# The AUTOUIC target property controls whether cmake(1) inspects the C++ files in the target to determine if they require uic to be run, and to create rules to execute uic at the appropriate time.
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)
#set(autouic_options
#  -tr tr2i18n
#  -include klocalizedstring.h
#)
#set_property(TARGET KI18n APPEND PROPERTY
#  INTERFACE_AUTOUIC_OPTIONS ${autouic_options}
#)
# The AUTORCC target property controls whether cmake(1) creates rules to execute rcc at the appropriate time on source files which have the suffix .qrc.
# add_executable(myexe main.cpp resource_file.qrc)

set(USE_QT4 OFF CACHE BOOL "use Qt4 or Qt5")

# make sure correct version installed!
if(USE_QT4)
  set(QT_VERSION_MAJOR 4.7)
  set(QT_VERSION_MINOR .0)
else(USE_QT4)
  set(QT_VERSION_MAJOR 5.12)
  set(QT_VERSION_MINOR .0)
endif(USE_QT4)

# The easiest way to use CMake is to set the CMAKE_PREFIX_PATH environment variable to the install prefix of Qt 5.
# https://stackoverflow.com/a/31223997/10904212
# list(APPEND CMAKE_PREFIX_PATH /opt/Qt${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/gcc_64)
prepend(CMAKE_PREFIX_PATH /opt/Qt${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/gcc_64)
message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
add_qt()

findPackageCrossPlatform(Threads REQUIRED)

# If you have built boost statically you will need to set the Boost_USE_STATIC_LIBS CMake variable to ON
#set(Boost_USE_STATIC_LIBS OFF) 
#set(Boost_USE_MULTITHREADED ON)  
#set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ROOT CACHE STRING /usr)
set(Boost_ADDITIONAL_VERSIONS="1.69.0")
#set(BOOST_INCLUDES CACHE STRING ${BOOST_ROOT}/include)
set(BOOST_LIBS CACHE STRING ${BOOST_ROOT}/lib)
findPackageCrossPlatform( Boost 1.69.0 COMPONENTS program_options filesystem regex date_time system thread graph REQUIRED )

option(USE_LUA "Use Lua (also called 'C' Lua) includes (default)" OFF)
option(USE_LUAJIT "Use LuaJIT includes instead of 'C' Lua ones (recommended, if you're using LuaJIT, but disabled by default)" ON)
set(USE_LUA_VERSION 5.1 CACHE STRING "Set the Lua version to use (default: 5.1)")

if(USE_LUAJIT)
  findPackageCrossPlatform(LuaJIT REQUIRED)
  set(USE_LUA OFF)
endif()

if(USE_LUA)
  findPackageCrossPlatform(Lua ${USE_LUA_VERSION} EXACT REQUIRED)
endif()

option(USE_G3LOG "Use g3log logger" ON)
add_g3log() # from Utils.cmake

option(USE_RANG "Use RANG for coloring terminal" ON)
add_rang() # from Utils.cmake

# g++ -DWEBRTC_POSIX -o simpleTest test2.cpp 
# TODO https://github.com/shakandrew/AgarPlusPlus/blob/abbd548ab1d0e0d908778baa9366fc3a83182f88/CMake/FindWebRTC.cmake
set(WEBRTC_SRC_PATH CACHE STRING "WEBRTC_SRC_PATH_HERE")
set(WEBRTC_TARGET_PATH CACHE STRING "WEBRTC_TARGET_PATH_HERE")
add_webrtc() # from Utils.cmake

## Search source files in folders
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/filesystem ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/config ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/lua ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/log ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/algorithm ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net/webrtc ${PROJECT_NAME} "" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/src/net/websockets ${PROJECT_NAME} "" )

set_vs_startup_project(${PROJECT_NAME}) # from Utils.cmake

set(CLANG_PATH CACHE STRING "/usr/lib/llvm-6.0/lib/clang/6.0.1")

set(WEBRTC_INCLUDES ${WEBRTC_SRC_PATH}/include
  ${WEBRTC_SRC_PATH}/include/webrtc
  ${WEBRTC_SRC_PATH}/third_party/abseil-cpp
  ${WEBRTC_SRC_PATH}/third_party/jsoncpp/source/include
  ${WEBRTC_SRC_PATH}/third_party/libyuv/include)

set(THIRDPARTY_FILES
  "lib/"
  "lib/whereami/"
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${LUA_INCLUDE_DIR}
  ${RANG_INCLUDE_DIR}
  ${G3LOG_INCLUDE_DIR}
  ${WEBRTC_INCLUDES}
  ${CLANG_PATH}/include)

set(THIRDPARTY_SOURCES
  "lib/whereami/whereami.c")

set(SOURCE_FILES ${THIRDPARTY_SOURCES} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HEADERS})

## Set global variables
#SET( ${PROJECT_NAME}_SRCS  "${${PROJECT_NAME}_SRCS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_HEADERS  "${${PROJECT_NAME}_HEADERS}" PARENT_SCOPE )
#SET( ${PROJECT_NAME}_DIRS  "${${PROJECT_NAME}_DIRS}" PARENT_SCOPE )

# OTHER_IDE_FILES for IDE even if they have no build rules.
set(${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA
  ".editorconfig"
  ".clang-format"
  ".gitignore")
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/cmake "${PROJECT_NAME}_OTHER_IDE_FILES" "cmake/*.cmake;cmake/*.imp" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/assets "${PROJECT_NAME}_OTHER_IDE_FILES" "assets/*.lua" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR}/client "${PROJECT_NAME}_OTHER_IDE_FILES" "client/*.js;client/*.html" )
addFolder( ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_NAME}_OTHER_IDE_FILES" "*.md" )

# Group source files in folders (IDE filters)
#assign_source_group("${SOURCE_FILES};${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA")

add_executable(${PROJECT_TARGET_EXE}
  ${SOURCE_FILES}
  ${${PROJECT_NAME}_OTHER_IDE_FILES_EXTRA})

target_include_directories(${PROJECT_TARGET_EXE} PUBLIC "src/")
target_include_directories(${PROJECT_TARGET_EXE} SYSTEM PRIVATE
  ${THIRDPARTY_FILES})

set_source_files_properties(
  ${OTHER_FILES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
)

# IWYU detects superfluous includes and when the include can be replaced with a forward declaration.
# It can be obtained using "apt-get install iwyu" or from "github.com/include-what-you-use".
# make sure it can find Clang built-in headers (stdarg.h and friends.)
# see https://stackoverflow.com/a/30951493/10904212
option(ENABLE_IWYU "ENABLE IWYU" ON) # same as: set(ENABLE_IWYU ON CACHE BOOL "ENABLE IWYU")
if (ENABLE_IWYU)
  set(IWYU_IMP "${CMAKE_SOURCE_DIR}/cmake/iwyu.imp")

  # Generate clang compilation database
  # see https://stackoverflow.com/a/31086619/10904212
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  find_package(PythonInterp) # Required by iwyu
  add_iwyu( ${PROJECT_TARGET_EXE} ) # from Utils.cmake
else()
  message(WARNING "iwyu turned off")
endif()

if(USE_QT4)
    target_link_libraries(${PROJECT_TARGET_EXE} PRIVATE Qt4::QtGui )
else(USE_QT4)
    target_link_libraries(${PROJECT_TARGET_EXE} PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets )
    # Qt5::Core Qt5::Network Qt5::Sql Qt5::Qml Qt5::Xml
endif(USE_QT4)

#get_target_property (QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
get_target_property (QT_CORE_IMPORTED_LOCATION Qt5::Core LOCATION)
message("QT_CORE_IMPORTED_LOCATION=${QT_CORE_IMPORTED_LOCATION}")

target_compile_definitions(${PROJECT_TARGET_EXE} PUBLIC
  ${WEBRTC_DEFINITIONS})
set_target_properties(${PROJECT_TARGET_EXE} PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CMAKE_CXX_STANDARD_REQUIRED ON
  CMAKE_CXX_FLAGS "-fno-rtti /std:c++latest"
  #IMPORTED_LOCATION "/opt/Qt${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/${QT_VERSION_MAJOR}${QT_VERSION_MINOR}/gcc_64/lib"
  #INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/libbar"
  RUNTIME_OUTPUT_DIRECTORY /${CMAKE_BINARY_DIR}/bin/)
# TODO ARCHIVE_OUTPUT_DIRECTORY
# TODO LIBRARY_OUTPUT_DIRECTORY

target_link_directories(${PROJECT_TARGET_EXE} PUBLIC ${WEBRTC_LIB_PATHS})

target_link_libraries(${PROJECT_TARGET_EXE} PRIVATE
  ${WEBRTC_LIBRARIES}
  boost_system
  pthread
  ssl
  crypto
  X11
  dl
  expat
  z
  stdc++fs
  ${Boost_LIBRARIES}
  ${LUA_LIBRARIES}
  ${G3LOG_LIBRARIES})

# TODO: support both CONFIG:Debug and CONFIG:Release

# TODO: copy QT shared libs
# @SEE https://github.com/mjoppich/bioGUI/blob/master/CMakeLists.txt#L453

add_custom_command(TARGET ${PROJECT_TARGET_EXE} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/ $<TARGET_FILE_DIR:${PROJECT_TARGET_EXE}>/assets)

#add_custom_target(project-related-files SOURCES .clang-format)

