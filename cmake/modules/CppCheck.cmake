################################################################################
#
# \file      cmake/CppCheck.cmake
# \copyright 2012-2015, J. Bakosi, 2016-2018, Los Alamos National Security, LLC.
# \brief     Setup target for code coverage analysis
#
################################################################################

find_program( CPPCHECK cppcheck )
find_program( CPPCHECK_HTMLREPORT cppcheck-htmlreport )

if(CPPCHECK AND CPPCHECK_HTMLREPORT)

  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/cppcheck)

  # Setup cppcheck static analysis target
  add_custom_target(cppcheck
    # Run cppcheck static analysis
    # TODO: --std=c++17
    COMMAND ${CPPCHECK} --xml --xml-version=2 --enable=all --std=c++14
    #        ${CMAKE_CURRENT_SOURCE_DIR} 2> doc/cppcheck/cppcheck-report.xml
            ${CPPCHECK_SOURCE_FILES} 2> doc/cppcheck/cppcheck-report.xml
    # Generate html output
    COMMAND ${CPPCHECK_HTMLREPORT} --file=doc/cppcheck/cppcheck-report.xml
            --report-dir=doc/cppcheck --source-dir=.
    # Set work directory for target
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    # Echo what is being done
    COMMENT "Quinoa cppcheck static analysis report"
  )

  # Output code coverage target enabled
  #string(REPLACE ";" " " ARGUMENTS "${ARG_TESTRUNNER_ARGS}")
  message(STATUS "Enabling cppcheck static analysis target 'cppcheck', report at ${CMAKE_BINARY_DIR}/doc/cppcheck/index.html")
else()
  message(WARNING "Can`t enable cppcheck static analysis")
endif()
