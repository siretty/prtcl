cmake_minimum_required( VERSION 3.5.1 )

project( prtcl-superbuild C CXX )

include( ExternalProject )

set( _EP_PREFIX       ${CMAKE_CURRENT_BINARY_DIR}/ep_prefix )
set( _EP_DOWNLOAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/archives )

# third party

#set( _GTEST_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/googletest )
#
#ExternalProject_Add(
#  googletest
#  PREFIX ${_EP_PREFIX} DOWNLOAD_DIR ${_EP_DOWNLOAD_DIR}/googletest
#  # download
#  URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz
#  URL_HASH SHA1=9c89be7df9c5e8cb0bc20b3c4b39bf7e82686770
#  # configure
#  CMAKE_CACHE_ARGS
#    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
#    # c language
#    -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
#    -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
#    # c++ language
#    -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
#    -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
#    # linker
#    -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
#    -DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
#    -DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}
#    -DCMAKE_STATIC_LINKER_FLAGS:STRING=${CMAKE_STATIC_LINKER_FLAGS}
#    # installation
#    -DCMAKE_INSTALL_PREFIX:PATH=${_GTEST_INSTALL_DIR}
#)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set( _BOOST_TOOLSET "clang" )
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  set( _BOOST_TOOLSET "gcc" )
else()
  message( WARNING "Unable to match ${CMAKE_CXX_COMPILER_ID} to a Boost.Build toolset." )
endif()

set( _BOOST_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost )

ExternalProject_Add(
  boost
  PREFIX ${_EP_PREFIX} DOWNLOAD_DIR ${_EP_DOWNLOAD_DIR}/boost
  URL https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.bz2
  URL_HASH SHA1=6d6ed02b29c860fd21b274fc4e1f820855e765e9
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND
    ./bootstrap.sh
      --prefix=<INSTALL_DIR>
      --with-toolset=${_BOOST_TOOLSET}
      --with-libraries=headers,container
  BUILD_COMMAND
    ./b2 install link=static variant=release threading=multi runtime-link=static
  INSTALL_COMMAND ""
  INSTALL_DIR ${_BOOST_INSTALL_DIR}
)

# projects

ExternalProject_Add(
  prtcl
  PREFIX ${_EP_PREFIX}
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.."
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/prtcl"
  INSTALL_COMMAND ""
  CMAKE_CACHE_ARGS
    # common cmake arguments
      -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
      # c language
      -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
      -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
      # c++ language
      -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
      # linker
      -DCMAKE_EXE_LINKER_FLAGS:STRING=${CMAKE_EXE_LINKER_FLAGS}
      -DCMAKE_MODULE_LINKER_FLAGS:STRING=${CMAKE_MODULE_LINKER_FLAGS}
      -DCMAKE_SHARED_LINKER_FLAGS:STRING=${CMAKE_SHARED_LINKER_FLAGS}
      -DCMAKE_STATIC_LINKER_FLAGS:STRING=${CMAKE_STATIC_LINKER_FLAGS}
    # third party
    #-DGTEST_ROOT:PATH=${CMAKE_CURRENT_BINARY_DIR}/third_party/googletest
    -DBOOST_ROOT:PATH=${_BOOST_INSTALL_DIR}
  DEPENDS boost
  BUILD_ALWAYS ON
)


