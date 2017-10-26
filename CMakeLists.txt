# +-------------------------------------------------------------------------
# | Licensed under the Apache License, Version 2.0 (the "License");
# | You may not use this work except in compliance with the License.
# | You may obtain a copy of the License in the LICENSE file, or at:
# |
# | http://www.apache.org/licenses/LICENSE-2.0
# |
# | Unless required by applicable law or agreed to in writing, software
# | distributed under the License is distributed on an "AS IS" BASIS,
# | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# | See the License for the specific language governing permissions and
# | limitations under the License.
# +-------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
if(CMAKE_MAJOR_VERSION LESS 3)
  message(
      WARNING "Building with CMake 3.0 or higher is strongly suggested;
      current version is ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.
      ${CMAKE_PATCH_VERSION}")
endif()

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
set(QSFS_VERSION "${VERSION_STRING}")

message(STATUS "Building project version: ${QSFS_VERSION}")
string(REPLACE "." ";" VERSION_LIST ${VERSION_STRING})
list (GET VERSION_LIST 0 QSFS_MAJOR_VERSION)
list (GET VERSION_LIST 1 QSFS_MINOR_VERSION)
list (GET VERSION_LIST 2 QSFS_PATCH_VERSION)

# 
# dependencies
#
# Using cmake scripts and modules
#list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
include(cmake/gtest.cmake)
include(cmake/gflags.cmake)
include(cmake/glog.cmake)

#
# Linter target
#
#include(cmake/lint.cmake)

#
# compile set up
#
#ADD_DEFINITIONS(-DENABLE_DEBUG -DABC)
##ifdef ENABLE_DEBUG /*code will be active #endif

# glog is built statically, and we want to link it into the qsfs's shared
# library, this requires position-independent code.
# As not all systems support '-shared' option to produce a shared
# object which can then ben linked with other object to form an executable.
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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# Where qsfs's .h files can be found.
include_directories(${PROJECT_SOURCE_DIR}/include)

# Where qsfs's targets can be found.
link_directories(${PROJECT_SOURCE_DIR}/lib)

# Where qingstor-cpp's .h files can be found
include_directories(${PROJECT_SOURCE_DIR}/external/include)
include_directories(${PROJECT_SOURCE_DIR}/external/include/qingstor-sdk-cpp)
# Where qingstor-cpp's libs
link_directories(${PROJECT_SOURCE_DIR}/external/lib)


#add_subdirectory(doc)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)
