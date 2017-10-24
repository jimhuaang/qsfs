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

#
# project
#
project (qsfs)

include(CTest)
enable_testing ()

#
# configure version
#
if(EXISTS ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h})
    FILE(REMOVE ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h})
endif()
configure_file(
  cmake/templates/VersionConfig.h.in
  "${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h")

# 
# dependencies
#
include(cmake/DownloadProject.cmake)
include(cmake/DownloadGTest.cmake)
include(cmake/DownloadGFlags.cmake)
include(cmake/DownloadGLog.cmake)

#
# Linter target
#
#include(cmake/lint.cmake)

#
# compile set up
#
#ADD_DEFINITIONS(-DENABLE_DEBUG -DABC)
##ifdef ENABLE_DEBUG /*code will be active #endif

add_compile_options(-g -std=c++11 -Wall -D_FILE_OFFSET_BITS=64)
option(
    BUILD_SHARED_LIBS "If enabled, all qsfs libraries will be build 
    as shared objects; otherwise all qsfs will be build as static objects" ON)
set(BUILD_SHARED_LIBS ON)
if(BUILD_SHARED_LIBS)
    set(LIBTYPE SHARED)
    message(STATUS "Building qsfs libraries as shared objects")
else(BUILD_SHARED_LIBS)
    set(LIBTYPE STATIC)
    message(STATUS "Building qsfs libraries as static objects")
endif(BUILD_SHARED_LIBS)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(QSFS_INCLUDES ${PROJECT_SOURCE_DIR}/include/)
include_directories(${QSFS_INCLUDES})
include_directories(${PROJECT_SOURCE_DIR}/external/include)
include_directories(${PROJECT_SOURCE_DIR}/external/include/qingstor-sdk-cpp)
set(QSFS_LIBS ${PROJECT_SOURCE_DIR}/lib/)
link_directories(${QSFS_LIBS})
link_directories(${PROJECT_SOURCE_DIR}/external/lib)

#
# sub dir
#
#add_subdirectory(doc)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(test)
