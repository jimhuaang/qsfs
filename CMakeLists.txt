cmake_minimum_required(VERSION 3.0)

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
set(QSFS_MAJOR_VERSION 1)
set(QSFS_MINOR_VERSION 0)
set(QSFS_PATCH_VERSION 0)
set(QSFS_VERSION ${QSFS_MAJOR_VERSION}.${QSFS_MINOR_VERSION}.${QSFS_PATCH_VERSION})
if(EXISTS ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h)
    FILE(REMOVE ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h)
endif()
configure_file(
  cmake/templates/VersionConfig.h.in
  "${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h")

message(STATUS "Building project version: ${QSFS_VERSION}")

# 
# dependencies
#
# Download and Install dependencies
include(cmake/gtest.cmake)
include(cmake/gflags.cmake)
include(cmake/glog.cmake)

# qingstor dependencies
#find_package(FindCURL REQUIRED)
#find_package(FindOpenSSL REQUIRED)
#find_package(ZLIB REQUIRED)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})
find_package(gflags REQUIRED)  # depended by glog
# refer to glog usage, no need to add include directory by hand.
# https://github.com/google/glog/blob/master/cmake/INSTALL.md
find_package(glog REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# do not specify -g, instead specify -DCMAKE_BUILD_TYPE=Debug/Release in command line
add_compile_options(-std=c++11 -Wall -Wunused-result -D_FILE_OFFSET_BITS=64)

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

#
# uninstall target
#
include(cmake/UninstallProject.cmake)
setup_uninstall_project(qsfs)

if (NOT TARGET uninstall)
    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P "${qsfs_BINARY_DIR}/cmake_uninstall.cmake"
        COMMAND ${CMAKE_COMMAND} -P "${googletest_BINARY_DIR}/cmake_uninstall.cmake"
        COMMAND ${CMAKE_COMMAND} -P "${googlelog_BINARY_DIR}/cmake_uninstall.cmake"
        COMMAND ${CMAKE_COMMAND} -P "${googleflags_BINARY_DIR}/cmake_uninstall.cmake")
endif()