include_guard( DIRECTORY )

macro(print_cmake_system_info)
  message(STATUS "Building ${PROJECT_NAME} project v${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")
  #set(CMAKE_CXX_FLAGS "-fno-rtti")
  message(STATUS "System: " ${CMAKE_SYSTEM_NAME} " " ${CMAKE_SYSTEM_VERSION})
  message(STATUS "Processor: " ${CMAKE_HOST_SYSTEM_PROCESSOR})
  message(STATUS "CMake generates: " ${CMAKE_GENERATOR})
  message(STATUS "Build type:" ${CMAKE_BUILD_TYPE})

  message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
  message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
  message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
  message(STATUS "CMAKE_STATIC_LIBRARY_SUFFIX: ${CMAKE_STATIC_LIBRARY_SUFFIX}")
endmacro(print_cmake_system_info)

macro(check_supported_os)
  if (NOT WIN32 AND NOT UNIX AND NOT APPLE)
    message(FATAL_ERROR "Unsupported operating system. Only Windows, Mac and Unix systems supported.")
  endif (NOT WIN32 AND NOT UNIX AND NOT APPLE)
endmacro(check_supported_os)

# Function to download zip files, then extracting them and then deleting them
function(download_zip FILE_NAME URL DOWNLOAD_PATH CHECKSUM)
  set(FULL_FILE_PATH "${DOWNLOAD_PATH}/${FILE_NAME}")
  if (NOT EXISTS ${FULL_FILE_PATH})
    message(STATUS "Downloading ${URL}...")
    file(DOWNLOAD "${URL}/${FILE_NAME}" "${DOWNLOAD_PATH}/${FILE_NAME}"
        EXPECTED_MD5 ${CHECKSUM})
    message(STATUS "Extracting ${FULL_FILE_PATH}...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${FILE_NAME} WORKING_DIRECTORY ${DOWNLOAD_PATH})
    else (NOT EXISTS ${FULL_FILE_PATH})
      message(STATUS "${FILE_NAME} already exists.")
  endif (NOT EXISTS ${FULL_FILE_PATH})
endfunction(download_zip)

# add item at the beginning of the list
function(prepend var prefix)
   set(listVar "")
   list(APPEND listVar "${prefix}")
   list(APPEND listVar ${${var}})
   list(REMOVE_DUPLICATES listVar)
   set(${var} "${listVar}" PARENT_SCOPE)
endfunction(prepend)

macro(set_project_version vmajor vminor vpatch)
  # The version number.
  set (${PROJECT_NAME}_VERSION_MAJOR vmajor)
  set (${PROJECT_NAME}_VERSION_MINOR vminor)
  set (${PROJECT_NAME}_VERSION_PATCH vpatch)
  set (${PROJECT_NAME}_VERSION "${vmajor}.${vminor}.${vpatch}")
endmacro(set_project_version)

macro(set_cmake_module_paths target module_paths)
  list(APPEND CMAKE_MODULE_PATH "${module_paths}")
  list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
  set( ${target}_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "${target}_CMAKE_MODULE_PATH" )
  message("CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")
endmacro(set_cmake_module_paths)

# Get names of subdirectories in directory
macro(subdirlist result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  foreach (child ${children})
    if (IS_DIRECTORY ${curdir}/${child} AND NOT ${child} STREQUAL "CMakeFiles")
      list(APPEND dirlist ${child})
    endif()
  endforeach ()
  set(${result} ${dirlist})
endmacro()

# Performs searching and adding of files to source list
# Appends source files to ${${PROJECT_NAME}_SRCS}
# Appends header files to ${${PROJECT_NAME}_HEADERS}
# Appends dir (argument) to ${${PROJECT_NAME}_DIRS}
# Appends extra_patterns (argument) to ${${PROJECT_NAME}_EXTRA}
# Example of extra_patterns: "cmake/*.cmake;cmake/*.imp"
macro(addFolder dir prefix extra_patterns)
  if (NOT EXISTS "${dir}")
    message(FATAL_ERROR "${dir} doesn`t exist!")
  endif()

  set(src_files "")
  set(header_files "")
  set(globType GLOB)
  if(${ARGC} GREATER 1 AND "${ARGV1}" STREQUAL "RECURSIVE")
      set(globType GLOB_RECURSE)
  endif()
  # Note: Certain IDEs will only display files that belong to a target, so add .h files too.
  file(${globType} src_files ABSOLUTE
      ${dir}/*.c
      ${dir}/*.cc
      ${dir}/*.cpp
      ${dir}/*.asm
      ${extra_patterns}
  )
  file(${globType} header_files ABSOLUTE
      ${dir}/*.h
      ${dir}/*.hpp
      ${extra_patterns}
  )
  file(${globType} extra_files ABSOLUTE
      ${extra_patterns}
  )
  LIST(APPEND ${prefix}_SRCS ${src_files})
  LIST(APPEND ${prefix}_HEADERS ${header_files})
  LIST(APPEND ${prefix}_EXTRA ${extra_files})
  LIST(APPEND ${prefix}_DIRS ${dir})
endmacro()

# Performs searching recursively and adding of files to source list
macro(addFolderRecursive dir prefix)
  addFolder("${dir}" "${prefix}" "" "RECURSIVE")
endmacro()

# This macro lets you find executable programs on the host system
# Usefull for emscripten
macro(find_host_package)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
  find_package(${ARGN})
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
endmacro(find_host_package)

# Useful cause some systems don`t allow easy package finding
macro(findPackageCrossPlatform)
  if(EMSCRIPTEN)
      find_host_package(${ARGN})
  elseif(ANDROID)
      find_host_package(${ARGN})
  elseif(CMAKE_HOST_WIN32)
      find_package(${ARGN})
  elseif(CMAKE_HOST_UNIX)
    find_package(${ARGN})
  else()
      message( "Unknown platform, using find_package" )
      find_package(${ARGN})
  endif()
endmacro(findPackageCrossPlatform)

# Group source files in folders (IDE filters)
function(assign_source_group)
  foreach(_source IN ITEMS ${ARGN})
      if (IS_ABSOLUTE "${_source}")
          file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
      else()
          set(source_rel "${_source}")
      endif()
      get_filename_component(_source_path "${_source_rel}" PATH)
      string(REPLACE "../../" "" _source_path "${_source_path}")
      string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
      source_group("${_source_path_msvc}" FILES "${_source}")
  endforeach()
endfunction(assign_source_group)

macro(add_qt)
  # find qt package
  if(USE_QT4)
    findPackageCrossPlatform(Qt4 4.7.0 REQUIRED)
    set(USE_QT5 OFF)
    #INCLUDE(${QT_USE_FILE})
    #ADD_DEFINITIONS(${QT_DEFINITIONS})
    #qt4_add_resources(reader_RSRC icons.qrc)
  else(USE_QT4)
    findPackageCrossPlatform(Qt5 ${QT_VERSION_MAJOR}${QT_VERSION_MINOR} REQUIRED COMPONENTS Core Gui Widgets)
    set(USE_QT5 ON)
    #find_package(Qt5Core)
    #find_package(Qt5Widgets REQUIRED)
    #qt5_add_resources(reader_RSRC icons.qrc)
  endif(USE_QT4)
endmacro()

macro(add_iwyu target_name)
  message("iwyu mapping_file at ${IWYU_IMP}")

  find_program(iwyu_path
    NAMES include-what-you-use iwyu
    PATHS
    ${CMAKE_SOURCE_DIR}/submodules/build-iwyu
    ${CMAKE_SOURCE_DIR}/include-what-you-use/build
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /usr/local/include
    /usr/local
    /usr
    /usr/bin/
    /opt/csw # Blastwave
    /opt)
  if(NOT iwyu_path)
    message(WARNING "Could not find the program include-what-you-use")
  else()
    # see https://github.com/aferrero2707/PhotoFlow/blob/master/src/external/rawspeed/cmake/iwyu.cmake#L7
    set(iwyu_path_and_options
        ${iwyu_path}
#       -Xiwyu --transitive_includes_only
        -Xiwyu --no_comments
        -Xiwyu --mapping_file=${IWYU_IMP}
        -Xiwyu --max_line_length=120
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*/*
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*/*/*
        -Xiwyu --check_also=${CMAKE_SOURCE_DIR}/src/*/*/*/*)
    #set_property(TARGET ${target_name} PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
    set_property(TARGET ${target_name} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
    message("iwyu FOUND at ${iwyu_path}")
  endif()

  # run with --target 'iwyu'
  #find_program(iwyu_tool_path NAMES iwyu_tool.py)
  #if (iwyu_tool_path AND PYTHONINTERP_FOUND)
  #  add_custom_target(iwyu
  #    ALL      # Remove ALL if you don't iwyu to be run by default.
  #    COMMAND "${PYTHON_EXECUTABLE}" "${iwyu_tool_path}" -p "${CMAKE_BINARY_DIR}" -- --mapping_file=${IWYU_IMP}
  #    COMMENT "Running include-what-you-use tool"
  #    VERBATIM
  #  )
  #endif()
endmacro(add_iwyu)

macro(set_vs_startup_project MY_PROJECT_NAME)
  message("startup_project: ${MY_PROJECT_NAME}")

  # Enable solution folders (in IDE)
  set_property( GLOBAL PROPERTY USE_FOLDERS ON )

  # New feature in CMake 3.6 lets us define the Visual Studio StartUp Project
  set_property(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      PROPERTY VS_STARTUP_PROJECT ${MY_PROJECT_NAME}
  )

  # Checks if PROPERTY VS_STARTUP_PROJECT works (for Visual Studio)
  if ( ${CMAKE_VERSION} VERSION_LESS "3.6" AND     # feature introduced in 3.6
          ${CMAKE_GENERATOR} MATCHES "Visual Studio") # we only care if generating for VS
          message("\n")
          message(WARNING
              "Visual Studio's \"StartUp Project\" cannot be set automatically by CMake ${CMAKE_VERSION}.\n"
              "Try upgrading to CMake 3.6 or use Visual Studio's solution explorer to manually set "
              "\"${MY_PROJECT_NAME}\" as the \"StartUp Project\".")
  endif()
endmacro(set_vs_startup_project)

macro(add_rang)
  if(USE_RANG)
    option(RANG_FIND_REQUIRED "RANG_FIND_REQUIRED" ON)
    findPackageCrossPlatform(Rang REQUIRED)
    message("RANG found at ${RANG_INCLUDE_DIR}")
  else()
    message(WARNING "RANG turned off!")
  endif()
endmacro(add_rang)

#macro(add_abseil)
#  if(USE_ABSEIL)
#    option(ABSEIL_FIND_REQUIRED "ABSEIL_FIND_REQUIRED" ON)
#    findPackageCrossPlatform(Abseil REQUIRED)
#    message("ABSEIL found at ${ABSEIL_INCLUDE_DIR}")
#  else()
#    message(WARNING "RANG turned off!")
#  endif()
#endmacro(add_abseil)

macro(add_g3log)
  if(USE_G3LOG)
    option(G3LOG_FIND_REQUIRED "G3LOG_FIND_REQUIRED" ON)
    findPackageCrossPlatform(g3log REQUIRED)
    message("g3log logger found at ${G3LOG_LIBRARIES} AND ${G3LOG_INCLUDE_DIR}")
  else()
    message(WARNING "g3log logger turned off!")
  endif()
endmacro(add_g3log)

macro(add_rapidjson)
  set( RAPIDJSON_DEFINITIONS RAPIDJSON_HAS_STDSTRING=1 RAPIDJSON_HAS_CXX11_RVALUE_REFS=1 )
endmacro(add_rapidjson)

macro(add_webrtc)
  if (WEBRTC_SRC_PATH)
  else()
    message(WARNING "using default WEBRTC_SRC_PATH")
    # NOTE: change WEBRTC_SRC_PATH
    set(WEBRTC_SRC_PATH "/home/denis/workspace/webrtc-checkout/src")
  endif()
  if (WEBRTC_TARGET_PATH)
  else()
    message(WARNING "using default WEBRTC_TARGET_PATH")
    set(WEBRTC_TARGET_PATH "out/release")
  endif()
  message("WEBRTC_SRC_PATH=${WEBRTC_SRC_PATH}")
  message("WEBRTC_TARGET_PATH=${WEBRTC_TARGET_PATH}")
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin"
    OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux"
    OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")

    set(WEBRTC_DEFINITIONS "-DWEBRTC_POSIX" CACHE INTERNAL "" FORCE)
    set(WEBRTC_LIBRARIES webrtc_full
      webrtc
      rtc_p2p
      jingle_peerconnection_api
      #boringssl
      CACHE INTERNAL "" FORCE)
    set(WEBRTC_LIB_PATHS ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}
      ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj
      ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/p2p
      ${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/api
      #${WEBRTC_SRC_PATH}/${WEBRTC_TARGET_PATH}/obj/third_party/boringssl
      CACHE INTERNAL "" FORCE)
  else()
  # TODO
  # #define WEBRTC_ANDROID 1
  # #define WEBRTC_IOS 1
  # #define WEBRTC_LINUX 1
  # #define WEBRTC_MAC 1
  # #define WEBRTC_WIN 1
  #
  # #define WEBRTC_POSIX 1
  message(FATAL_ERROR "System ${CMAKE_SYSTEM_NAME} not supported")
  endif()
endmacro(add_webrtc)

# Path to root dir (absolute)
# get_filename_component(ABSOLUTE_ROOT_DIR "${EXA_ROOT}" ABSOLUTE)
