cmake_minimum_required(VERSION 3.0)

option(BUILD_SHARED_LIBS "Build shared libraries." ON)

option(QSFS_BUILD_TESTS "Build all of qsfs's own tests." ON)

#
# project
#
project (qsfs)

if (QSFS_BUILD_TESTS)
  include(CTest)
  enable_testing ()
endif (QSFS_BUILD_TESTS)

#
# configure version
#
set(QSFS_VERSION 1.0.0)
if(EXISTS ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h})
    FILE(REMOVE ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h})
endif()
configure_file(
  cmake/templates/VersionConfig.h.in
  "${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h")

# extract version string from the existing generated header file
file(
    STRINGS "${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h" 
    __QSFS_VERSION_LINE LIMIT_COUNT 1 
    REGEX "QSFS_VERSION_STRING.*[0-9]+\\.[0-9]+\\.[0-9]+")
string(
  REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)"
  VERSION_STRING "${__QSFS_VERSION_LINE}")

message(STATUS "Building project version: ${QSFS_VERSION}")
string(REPLACE "." ";" VERSION_LIST ${VERSION_STRING})
list (GET VERSION_LIST 0 QSFS_MAJOR_VERSION)
list (GET VERSION_LIST 1 QSFS_MINOR_VERSION)
list (GET VERSION_LIST 2 QSFS_PATCH_VERSION)

# 
# dependencies
#
# Using cmake scripts
include(cmake/gtest.cmake)
include(cmake/gflags.cmake)
include(cmake/glog.cmake)

#
# linter target (make lint)
#
add_custom_target(lint COMMAND ${CMAKE_COMMAND} -P
                  ${CMAKE_SOURCE_DIR}/cmake/lint.cmake)

#
# compile set up
#

# As we build gflags as a subproject and gflags is built as static single-threaded
# library by default. As glog depend on the gflags library and should link to gfalgs
# target, and this requires position-independent code.
# As not all systems support '-shared' option to produce a shared object
# which can then be linked with other object to form an executable.
# For predictable results, explicitly specify the -fPIC option.
# For example, on Debain (gcc 6.3.0), without explicit set -fPIC, cmake
# raise error: 
#   EXPORT glog-target requires target gflags_nothreads_static 
#   which is not in export set.
add_compile_options(-fPIC)

add_compile_options(-g -std=c++11 -Wall -D_FILE_OFFSET_BITS=64)

if(BUILD_SHARED_LIBS)
    set(LIBTYPE SHARED)
else(BUILD_SHARED_LIBS)
    set(LIBTYPE STATIC)
endif(BUILD_SHARED_LIBS)

# Where qsfs's .h files can be found.
include_directories(${PROJECT_SOURCE_DIR}/include)

# Where qsfs's targets can be found.
link_directories(${PROJECT_SOURCE_DIR}/lib)

# Where qingstor-cpp's .h files can be found
include_directories(${PROJECT_SOURCE_DIR}/external/include)
include_directories(${PROJECT_SOURCE_DIR}/external/include/qingstor-sdk-cpp)
# Where qingstor-cpp's libs
link_directories(${PROJECT_SOURCE_DIR}/external/lib)


add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)
