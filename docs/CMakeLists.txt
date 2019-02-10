
cmake_minimum_required( VERSION 3.13.3 FATAL_ERROR )

find_package(Doxygen)
if(NOT DOXYGEN_FOUND)
  message(FATAL_ERROR "Doxygen not found, unable to generate documentation")
endif()

set(root_dir "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(doc_target_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(doxy_target_dir "${CMAKE_CURRENT_BINARY_DIR}")

set(doxy_template "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
set(doxy_file "${doc_target_dir}/Doxyfile")
set(doxy_mcss_file "${doc_target_dir}/Doxyfile-mcss")

# @note: m.css theme cloned as submodule
set(m_css_executable "${root_dir}/submodules/m.css/doxygen/dox2html5.py")

## START used in the Doxyfile.in >
set(DOXY_DOC_DEST_DIR ${doc_target_dir} CACHE PATH "Path to the doxygen output")
set(DOXY_PROJECT_NAME ${PROJECT_NAME} CACHE PATH "DOXY_PROJECT_NAME")
set(DOXY_PROJECT_VER ${${PROJECT_NAME}_VERSION} CACHE PATH "DOXY_PROJECT_VER")
set(DOXY_DOC_INPUT_ROOT_DIRS "${root_dir}/src/ ${root_dir}/docs" CACHE PATH "Path from where to start to parse")
set(DOXY_DOC_EXCLUDE_PATTERNS_DIRS ${doc_target_dir} CACHE STRING "Regex patterns exclusion")
set(DOXY_DOC_COMMON_IMG_PATH "${root_dir}" CACHE PATH "Path from which doxygen will be able to found referenced images")
## END used in the Doxyfile.in <

# Add a hint to help Cmake to find the correct python version:
# (see https://cmake.org/cmake/help/v3.0/module/FindPythonInterp.html)
set(Python_ADDITIONAL_VERSIONS 3)
# You can set -DPYTHON_EXECUTABLE=/usr/bin/python3
find_package(PythonInterp 3.6 REQUIRED)
if(NOT PYTHONINTERP_FOUND)
  message(FATAL_ERROR "Python 3 not found, unable to generate documentation")
endif()

message(STATUS "doc_target_dir: ${doc_target_dir}")

# create dirs
add_custom_command(
  OUTPUT ${doc_target_dir}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${doc_target_dir}
  COMMENT "Creating documentation directory for ${CMAKE_PROJECT_NAME}"
)

# configure Doxyfile.in
add_custom_command(
  OUTPUT ${doxy_file} # this line links the command to below add_custom_target
  COMMAND ${CMAKE_COMMAND}
          -D "DOXY_TEMPLATE=${doxy_template}"
          -D "DOXY_DOC_DEST_DIR=${DOXY_DOC_DEST_DIR}"
          -D "DOXY_PROJECT_NAME=${DOXY_PROJECT_NAME}"
          -D "DOXY_PROJECT_VER=${DOXY_PROJECT_VER}"
          -D "DOXY_DOC_INPUT_ROOT_DIRS=${DOXY_DOC_INPUT_ROOT_DIRS}"
          -D "DOXY_DOC_EXCLUDE_PATTERNS_DIRS=${DOXY_DOC_EXCLUDE_PATTERNS_DIRS}"
          -D "DOXY_DOC_COMMON_IMG_PATH=${DOXY_DOC_COMMON_IMG_PATH}"
          -D "DOXY_FILE=${doxy_file}"
          -D "DOXY_ROOT_DIR=${root_dir}"
          -D "DOXY_STRIP_FROM_PATH=${root_dir}"
          -D "DOXY_OUTPUT_DIR=${doc_target_dir}"
          -P ${root_dir}/cmake/configure_doxygen.cmake
  DEPENDS ${doc_target_dir}
  COMMENT "Generating Doxyfile for ${CMAKE_PROJECT_NAME}"
)

# copy DOXYMCSS_SRC to DOXYMCSS_DST
add_custom_command(
  OUTPUT ${doxy_mcss_file}
  COMMAND ${CMAKE_COMMAND}
          -D "DOXYMCSS_SRC=${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile-mcss"
          -D "DOXYMCSS_DST=${doc_target_dir}"
          -P ${root_dir}/cmake/create_doxy_mcss.cmake
  COMMENT "Generating Doxyfile-mcss for ${CMAKE_PROJECT_NAME}"
)

# build docs with mcss theme
add_custom_command(
  OUTPUT ${doxy_target_dir}/html
  COMMAND ${PYTHON_EXECUTABLE} ${m_css_executable} --debug ${doxy_mcss_file}
  WORKING_DIRECTORY ${root_dir}
  DEPENDS ${doxy_file} ${doxy_mcss_file}
  COMMENT "Creating documentation for ${CMAKE_PROJECT_NAME}"
)

# build docs in standard theme
add_custom_target(doxyDoc_notheme COMMAND ${DOXYGEN_EXECUTABLE} "${doc_target_dir}/Doxyfile"
  WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
  DEPENDS ${doxy_file} ${doxy_mcss_file}
  COMMENT "Building user's documentation into doxyDoc build dir..."
)

# build both mcss and standard theme
add_custom_target(doxyDoc ALL
  DEPENDS
  ${doc_target_dir}
  ${doxy_file}
  ${doxy_mcss_file}
  ${doxy_target_dir}/html
  # doxyDoc_notheme
)

install(DIRECTORY ${doc_target_dir} DESTINATION share/doc COMPONENT docs OPTIONAL) # OPTIONAL means do not raise an error if target file/dir not found
