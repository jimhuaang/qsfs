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

set(QSFS_MAJOR_VERSION 1)
set(QSFS_MINOR_VERSION 0)
set(QSFS_PATCH_VERSION 0)
set(QSFS_VERSION
  ${QSFS_MAJOR_VERSION}.${QSFS_MINOR_VERSION}.${QSFS_PATCH_VERSION})

#option (WITH_THREADS "Enable multithreading support" ON)
option(BUILD_SHARED_LIBS "If enabled, all qsfs libraries will be build as shared objects;
otherwise all qsfs will be build as static objects" ON)

#
# download gtest
#
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


#
# compile set up
#
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(QSFS_INCLUDES ${PROJECT_SOURCE_DIR}/include/)
include_directories(${QSFS_INCLUDES})

set(QSFS_LIBS ${PROJECT_SOURCE_DIR}/lib/)
link_directories(${QSFS_LIBS})

include_directories(${PROJECT_SOURCE_DIR}/external/include)
include_directories(${PROJECT_SOURCE_DIR}/external/include/qingstor-sdk-cpp)
link_directories(${PROJECT_SOURCE_DIR}/external/lib)

add_compile_options(-g -std=c++11 -Wall -D_FILE_OFFSET_BITS=64)
#add_compile_options(--enable-threads)
#add_compile_options(-I${PROJECT_SOURCE_DIR}/external/include/)
#add_compile_options(-I${PROJECT_SOURCE_DIR}/external/include/qingstor-sdk-cpp)
#add_compile_options(-L${PROJECT_SOURCE_DIR}/external/lib/)

#target_include_directories ( ${PROJECT_NAME} BEFORE PUBLIC
#  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
#  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
#  "$<INSTALL_INTERFACE:include>"
#  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
#  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)


#
# make libs
#

# configure
add_library(
  Configure
  src/configure/Default.cpp
  src/configure/Options.cpp
  )
target_link_libraries(Configure fuse)

# base
add_library(
  Logging
  src/base/Logging.cpp 
  src/base/LogLevel.cpp
  )
target_link_libraries(Logging gtest glog Configure)  # FRIEND_TEST

add_library(
  ThreadPool
  src/base/ThreadPool.cpp
  src/base/TaskHandle.cpp
  )
target_link_libraries(ThreadPool gtest)  # FRIEND_TEST

add_library(
  BaseUtils
  src/base/StringUtils.cpp
  src/base/TimeUtils.cpp
  src/base/Utils.cpp
  )
target_link_libraries(BaseUtils Logging)

# data
add_library(
  Directory 
  src/data/Directory.cpp 
  src/data/FileMetaData.cpp
  src/data/FileMetaDataManager.cpp
  )
target_link_libraries(Directory gtest)  # FRIEND_TEST

add_library(
  Cache
  src/data/Cache.cpp
  src/data/File.cpp
  src/data/Page.cpp
  )
target_link_libraries(Cache gtest)  # FRIEND_TEST

add_library(
  Resource
  src/data/IOStream.cpp
  src/data/ResourceManager.cpp
  src/data/StreamBuf.cpp
  src/data/StreamUtils.cpp
  )
target_link_libraries(Resource gtest)  # FRIEND_TEST

# client
add_library(
  Client 
  src/client/Client.cpp
  src/client/ClientConfiguration.cpp
  src/client/ClientImpl.cpp
  src/client/ClientFactory.cpp
  src/client/Credentials.cpp
  src/client/NullClient.cpp
  src/client/Protocol.cpp
  src/client/QSClientImpl.cpp 
  src/client/QSClient.cpp 
  src/client/QSClientConverter.cpp
  src/client/QSError.cpp
  src/client/RetryStrategy.cpp
  src/client/URI.cpp
  src/client/Utils.cpp
  src/client/Zone.cpp
  )
target_link_libraries(Client Logging qingstor)

add_library(
  Transfer
  src/client/QSTransferManager.cpp
  src/client/TransferHandle.cpp
  src/client/TransferManager.cpp
  src/client/TransferManagerFactory.cpp
  )
target_link_libraries(Transfer Logging)

# filesystem
add_library(
  FileSystem
  src/filesystem/Drive.cpp
  src/filesystem/HelpText.cpp
  src/filesystem/Initializer.cpp
  src/filesystem/Mounter.cpp
  src/filesystem/MimeTypes.cpp
  src/filesystem/Operations.cpp
  src/filesystem/Parser.cpp
  src/filesystem/RegisterInitializers.cpp
)
target_link_libraries(FileSystem fuse Logging)

#
# main
#

file(GLOB QSFS_HEADERS ${PROJECT_SOURCE_DIR}/include/*/*.h)
file(GLOB QSFS_SOURCES ${PROJECT_SOURCE_DIR}/src/*/*.cpp)
file(GLOB QSFS_SRC ${QSFS_HEADERS} ${QSFS_SOURCES})
add_executable(
  ${PROJECT_NAME} 
  src/main.cpp
  ${QSFS_SRC}
  )
target_link_libraries(${PROJECT_NAME} fuse glog gtest qingstor)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${QSFS_VERSION})
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
#target_link_libraries(${PROJECT_NAME} FileSystem)
#target_link_libraries(${PROJECT_NAME} Client Transfer )
#target_link_libraries(${PROJECT_NAME} Cache Resource Directory)
#target_link_libraries(${PROJECT_NAME} Configure)
#target_link_libraries(${PROJECT_NAME} BaseUtils ThreadPool Logging)

add_executable(sample1 test/sample1.cpp)
target_link_libraries(sample1 gtest)
add_test(NAME sample1_test COMMAND sample1)

#
# unit testing
#

# Unit testing
#if (BUILD_TESTING)
#  add_executable ( ${PROJECT_NAME}_unittest
#    src/test/QSFSUnitTest.cpp
#  )
#endif (BUILD_TESTING)
enable_testing ()
include(CTest)

add_executable(ExceptionTest test/ExceptionTest.cpp)
target_link_libraries(ExceptionTest gtest)
add_test(NAME exception_test COMMAND ExceptionTest)

add_executable(
  LoggingTest
  test/LoggingTest.cpp 
  )
target_link_libraries(LoggingTest gtest Logging BaseUtils)
add_test(NAME logging_test COMMAND LoggingTest)

add_executable(
  ThreadPoolTest 
  test/ThreadPoolTest.cpp
  )
target_link_libraries(ThreadPoolTest ThreadPool gtest)
add_test(NAME threadpool_test COMMAND ThreadPoolTest)

add_executable(
  DirectoryTest
  test/DirectoryTest.cpp
  )
target_link_libraries(DirectoryTest gtest Directory)
target_link_libraries(DirectoryTest Logging BaseUtils)
add_test(NAME directory_test COMMAND DirectoryTest)

add_executable(
  FileMetaDataManagerTest
  test/FileMetaDataManagerTest.cpp
  )
target_link_libraries(FileMetaDataManagerTest gtest Directory)
target_link_libraries(FileMetaDataManagerTest Logging BaseUtils)
add_test(NAME metadata_manager_test COMMAND FileMetaDataManagerTest)

add_executable(
  StreamTest
  test/StreamTest.cpp
  )
target_link_libraries(StreamTest gtest Resource)
target_link_libraries(StreamTest Logging BaseUtils)
add_test(NAME stream_test COMMAND StreamTest)

add_executable(
  ResourceManagerTest
  test/ResourceManagerTest.cpp
  )
target_link_libraries(ResourceManagerTest gtest Resource)
target_link_libraries(ResourceManagerTest Logging BaseUtils)
add_test(NAME resource_manager_test COMMAND ResourceManagerTest)

add_executable(
  PageTest
  test/PageTest.cpp
  )
target_link_libraries(PageTest gtest Cache Resource)
target_link_libraries(PageTest Logging BaseUtils)
add_test(NAME page_test COMMAND PageTest)

add_executable(
  FileTest
  test/FileTest.cpp
  )
target_link_libraries(FileTest gtest Cache Resource)
target_link_libraries(FileTest Logging BaseUtils)
add_test(NAME file_test COMMAND FileTest)

add_executable(
  CacheTest
  test/CacheTest.cpp
  )
target_link_libraries(CacheTest gtest Cache Resource)
target_link_libraries(CacheTest Logging BaseUtils)
add_test(NAME cache_test COMMAND CacheTest)

#
# install
#
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
