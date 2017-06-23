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
  message(WARNING "Building with CMake 3.0 or higher is strongly suggested; current version is ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
endif()
project(qsfs)

enable_testing ()

set(QSFS_MAJOR_VERSION 0)
set(QSFS_MINOR_VERSION 1)
set(QSFS_PATCH_VERSION 1)

set(QSFS_VERSION
  ${QSFS_MAJOR_VERSION}.${QSFS_MINOR_VERSION}.${QSFS_PATCH_VERSION})

#option (WITH_THREADS "Enable multithreading support" ON)
option(BUILD_SHARED_LIBS "If enabled, all qsfs libraries will be build as shared objects;
otherwise all qsfs will be build as static objects" ON)

include(CTest)

if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadProject.cmake)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

# When using CMake 2.8.11 or later, header path dependencies
# are automatically added to the gtest and gmock targets.
# For earlier CMake versions, we have to explicitly add the
# required directories to the header search path ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include"
                        "${gmock_SOURCE_DIR}/include")
endif()


#list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#include (CheckCXXCompilerFlag)
#include (CheckCXXSourceCompiles)
#include (CheckIncludeFile)
#include (CheckIncludeFileCXX)
#include (CTest)

#if (WITH_THREADS)
#  find_package (Threads)
#endif (WITH_THREADS)

#check_include_file (stdint.h HAVE_STDINT_H)
#check_include_file (stdlib.h HAVE_STDLIB_H)
#check_include_file (string.h HAVE_STRING_H)
#check_include_file (sys/stat.h HAVE_SYS_STAT_H)
#check_include_file (sys/time.h HAVE_SYS_TIME_H)
#check_include_file (sys/types.h HAVE_SYS_TYPES_H)
#check_include_file (unistd.h HAVE_UNISTD_H)

#check_include_file_cxx ("unordered_map" HAVE_UNORDERED_MAP)

#check_type_size (uint64_t HAVE_UINT64_T)


#set (TEST_SRC_DIR \"${CMAKE_CURRENT_SOURCE_DIR}\")

#configure_file (src/config.h.cmake.in config.h)

file(GLOB QSFS_HEADERS "include/*.h")
file(GLOB QSFS_BASE_HEADERS "include/base/*.h")
MESSAGE(STATUS "QSFS_HEADERS: ${QSFS_HEADERS}")

file(GLOB QSFS_SOURCES "src/*.cpp")
file(GLOB QSFS_BASE_SOURCES "src/*.cpp")

file(GLOB QSFS_UNIFIED_HEADERS
  ${QSFS_HEADERS}
  ${QSFS_BASE_HEADERS}
)

file(GLOB QSFS_UNIFIED_SOURCES
  ${QSFS_SOURCES}
  ${QSFS_BASE_SOURCES}
)

file(GLOB QSFS_SRC
  ${QSFS_UNIFIED_HEADERS}
  ${QSFS_UNIFIED_SOURCES}
)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(QSFS_INCLUDES ${PROJECT_SOURCE_DIR}/include/)
include_directories(${QSFS_INCLUDES})

add_compile_options(-g -std=c++11)

add_library(${PROJECT_NAME} ${QSFS_SRC})

#set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${QSFS_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

#target_include_directories ( ${PROJECT_NAME} BEFORE PUBLIC
#  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
#  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
#  "$<INSTALL_INTERFACE:include>"
#  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
#  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Unit testing
#if (BUILD_TESTING)
#  add_executable ( ${PROJECT_NAME}_unittest
#    src/test/QSFSUnitTest.cpp
#  )
#endif (BUILD_TESTING)

add_library(FileSystem src/base/FileSystem.cpp)
add_library(Logging src/base/Logging.cpp src/base/LogLevel.cpp)
add_library(ThreadPool src/base/ThreadPool.cpp src/base/TaskHandle.cpp)
add_library(Semaphore src/base/Semaphore.cpp)
add_library(QSErrors src/client/QSErrors.cpp)

add_executable(sample1 test/sample1.cpp)
target_link_libraries(sample1 gtest)
add_test(NAME sample1_test COMMAND sample1)

add_executable(LoggingTest test/LoggingTest.cpp 
               src/base/Logging.cpp 
               src/base/LogLevel.cpp)
target_link_libraries(LoggingTest gtest glog)
add_test(NAME logging_test COMMAND LoggingTest)

add_executable(FileSystemTest test/FileSystemTest.cpp 
               src/base/FileSystem.cpp)
target_link_libraries(FileSystemTest gtest)
add_test(NAME filesystem_test COMMAND FileSystemTest)

install (TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib/static
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install (FILES ${QSFS_HEADERS} DESTINATION include/${PROJECT_NAME})
install (FILES ${QSFS_BASE_HEADERS} DESTINATION include/${PROJECT_NAME}/base)

install (DIRECTORY doc/ DESTINATION share/doc/${PROJECT_NAME})


#configure_package_config_file (${PROJECT_NAME}-config.cmake.in
#  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
#  INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
#  NO_CHECK_REQUIRED_COMPONENTS_MACROS)

#export (TARGETS ${PROJECT_NAME} NAMESPACE QS:: FILE ${PROJECT_NAME}-targets.cmake)
#export (PACKAGE ${PROJECT_NAME})

#install (FILES
#  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
#  DESTINATION lib/cmake/${PROJECT_NAME})

#install (EXPORT ${PROJECT_NAME} NAMESPACE QS:: DESTINATION lib/cmake/${PROJECT_NAME})

#add_executable (${PROJECT_NAME}
#  src/main.c
#)
