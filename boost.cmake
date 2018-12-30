# TODO

include( ExternalProject )

set( boost_URL "http://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.tar.bz2" )
set( boost_SHA1 "9f1dd4fa364a3e3156a77dc17aa562ef06404ff6" )
set( boost_INSTALL ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost )
set( boost_INCLUDE_DIR ${boost_INSTALL}/include )
set( boost_LIB_DIR ${boost_INSTALL}/lib )

ExternalProject_Add( boost
        PREFIX boost
        URL ${boost_URL}
        URL_HASH SHA1=${boost_SHA1}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND
        ./bootstrap.sh
        --with-libraries=filesystem
        --with-libraries=system
        --with-libraries=date_time
        --prefix=<INSTALL_DIR>
        BUILD_COMMAND
        ./b2 install link=static variant=release threading=multi runtime-link=static
        INSTALL_COMMAND ""
        INSTALL_DIR ${boost_INSTALL} )

set( Boost_LIBRARIES
        ${boost_LIB_DIR}/libboost_filesystem.a
        ${boost_LIB_DIR}/libboost_system.a
        ${boost_LIB_DIR}/libboost_date_time.a )
message( STATUS "Boost static libs: " ${Boost_LIBRARIES} )