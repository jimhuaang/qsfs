cmake_minimum_required(VERSION 3.0)

#
# options
#
# to turn on, specify -DBUILD_TESTS=ON in cmake command line
option (BUILD_TESTS "Enable build of the unit tests and their execution using CTest." OFF)
option (BUILD_PACKAGING "Enable build of distribution packages using CPack." OFF)
option (INSTALL_HEADERS "Request installation of headers and other development files." ON)
option (REGISTER_BUILD_DIR "Request entry of build directory in CMake's package registry." OFF)

#
# project information
#
set (PACKAGE_NAME        "qsfs")
set (PACKAGE_VERSION_MAJOR 1)
set (PACKAGE_VERSION_MINOR 0)
set (PACKAGE_VERSION_PATCH 0)
set (PACKAGE_VERSION     ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH})
set (PACKAGE_STRING      "${QSFS_NAME} ${QSFS_VERSION}")
set (PACKAGE_TARNAME     "${QSFS_NAME}-${QSFS_VERSION}")
set (PACKAGE_BUGREPORT   "https://github.com/jimhuaang/qsfs/issues")
set (PACKAGE_DESCRIPTION "A FUSE based filesystem allows you to mount a qingstor bucket in Linux.")
set (PACKAGE_URL         "https://github.com/jimhuaang/qsfs")

project (${PACKAGE_NAME})

# prefix for package variables in CMake configuration file
string (TOUPPER "${PACKAGE_NAME}" PACKAGE_PREFIX)


#
# configure version
#
if (EXISTS ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h)
    FILE(REMOVE ${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h)
endif ()
configure_file (
  cmake/templates/VersionConfig.h.in
  "${CMAKE_SOURCE_DIR}/include/configure/VersionConfig.h")

message (STATUS "Building project version: ${PACKAGE_VERSION}")


# 
# dependencies
#
find_package (Threads REQUIRED)
if (NOT Threads_FOUND)
  message (FATAL_ERROR "Could not find pthread. Check the log file"
  "\n\t${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log")
endif ()

# Download and Install dependencies
find_package (GTest)
if (NOT GTest_FOUND)
  include (cmake/gtest.cmake)
endif()
include_directories (${GTEST_INCLUDE_DIRS})

find_package (gflags)  # depended by glog
if(NOT GFLAGS_INCLUDE_DIR)
  include (cmake/gflags.cmake)
endif()

find_package (Glog)
if(NOT TARGET glog::glog)
  include (cmake/glog.cmake)
endif(NOT TARGET glog::glog)

# always download and intall sdk, no add FindQingStorSDK.cmake
include (cmake/qingstorsdk.cmake)

# qingstor dependencies
#find_package (FindCURL REQUIRED)
#find_package (FindOpenSSL REQUIRED)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


#
# linter target (make lint)
#
add_custom_target (lint COMMAND ${CMAKE_COMMAND} -P
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
add_compile_options (-fPIC)

# do not specify -g, instead specify -DCMAKE_BUILD_TYPE=Debug/Release in command line
add_compile_options (-std=c++11 -Wall -Wunused-result -D_FILE_OFFSET_BITS=64)


#
# set up include directories
#
# Where qsfs's .h files can be found.
include_directories (${PROJECT_SOURCE_DIR}/include)

# Where qingstor-cpp's .h files can be found
include_directories(/usr/local/include/qingstor)

# Where qingstor-cpp's libs
link_directories(/usr/local/lib)
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPAHT
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


#
# installation
#
# directories vars should be put before include directories
set (INSTALL_RUNTIME_DIR bin CACHE PATH "Installation directory for executables")
set (INSTALL_LIBRARY_DIR lib CACHE PATH "Installation directory for libraries")
set (INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
set (INSTALL_CMAKE_DIR  ${INSTALL_LIBRARY_DIR}/cmake/${PROJECT_NAME} CACHE PATH 
  "Installation directory for CMake files")

set (INSTALL_PREFIX_REL2CONFIG_DIR .)
set (EXPORT_NAME ${PROJECT_NAME}-targets)
file (RELATIVE_PATH INSTALL_PREFIX_REL2CONFIG_DIR
  "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}" "${CMAKE_INSTALL_PREFIX}")

# for build tree
configure_file (cmake/templates/config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" @ONLY)
# for install tree
configure_file (cmake/templates/config.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config.cmake" @ONLY)
# for both
configure_file(cmake/templates/version.cmake.in 
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake" @ONLY)

if (INSTALL_HEADERS)
  install (FILES
    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config.cmake"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
  )
  install (
    EXPORT ${EXPORT_NAME}
    DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev
  )
endif ()

# support direct use of build tree
export (TARGETS ${TARGETS} FILE "${PROJECT_BINARY_DIR}/${EXPORT_NAME}.cmake")
if (REGISTER_BUILD_DIR)
  export (PACKAGE ${PACKAGE_NAME})
endif ()


#
# uninstall target
#
include(cmake/UninstallProject.cmake)
setup_uninstall_project(${PROJECT_NAME})

set (UNINSTALL_ALL uninstall_${PROJECT_NAME})

if(TARGET uninstall_qingstorsdk)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_qingstorsdk)
endif()

if(TARGET uninstall_googletest)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_googletest)
endif()

if(TARGET uninstall_googlelog)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_googlelog)
endif()

if(TARGET uninstall_googleflags)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_googleflags)
endif()

add_custom_target(
  uninstall DEPENDS ${UNINSTALL_ALL}
)


#
# subdirectories
#
add_subdirectory (include)
add_subdirectory (src)


#
# testing
#
if (BUILD_TESTS)
  include(CTest)
  enable_testing ()
  add_subdirectory(test)
endif (BUILD_TESTS)

#
# packaging
#
if (BUILD_PACKAGING)
  include (InstallRequiredSystemLibraries)

  # unix
  set (PACKAGE_GENERATOR        "DEB;RPM")
  set (PACKAGE_SOURCE_GENERATOR "TGZ;ZIP")

  # used package generators
  set (CPACK_GENERATOR        "${PACKAGE_GENERATOR}"        CACHE STRING "List of binary package generators (CPack).")
  set (CPACK_SOURCE_GENERATOR "${PACKAGE_SOURCE_GENERATOR}" CACHE STRING "List of source package generators (CPack).")
  mark_as_advanced (CPACK_GENERATOR CPACK_SOURCE_GENERATOR)

  # common package information
  set (CPACK_PACKAGE_VENDOR              "yunify")
  set (CPACK_PACKAGE_CONTACT             "jimhuang@yunify.com")
  set (CPACK_PACKAGE_NAME                "${PACKAGE_NAME}")
  set (CPACK_PACKAGE_VERSION             "${PACKAGE_VERSION}")
  set (CPACK_PACKAGE_VERSION_MAJOR       "${PACKAGE_VERSION_MAJOR}")
  set (CPACK_PACKAGE_VERSION_MINOR       "${PACKAGE_VERSION_MINOR}")
  set (CPACK_PACKAGE_VERSION_PATCH       "${PACKAGE_VERSION_PATCH}")
  set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE_DESCRIPTION}")
  set (CPACK_RESOURCE_FILE_WELCOME       "${PROJECT_SOURCE_DIR}/README.md")
  set (CPACK_RESOURCE_FILE_LICENSE       "${PROJECT_SOURCE_DIR}/COPYING")
  set (CPACK_PACKAGE_DESCRIPTION_FILE    "${PROJECT_SOURCE_DIR}/README.md")
  set (CPACK_INSTALL_PREFIX              "${CMAKE_INSTALL_PREFIX}")
  set (CPACK_OUTPUT_FILE_PREFIX          packages)
  set (CPACK_PACKAGE_RELOCATABLE         TRUE)
  set (CPACK_MONOLITHIC_INSTALL          TRUE)

  # RPM package information -- used in cmake/package.cmake.in also for DEB
  set (CPACK_RPM_PACKAGE_GROUP   "Development/Libraries")
  set (CPACK_RPM_PACKAGE_LICENSE "Apache")
  set (CPACK_RPM_PACKAGE_URL     "${PACKAGE_URL}")
  set (CPACK_RPM_CHANGELOG_FILE  "${PROJECT_SOURCE_DIR}/ChangeLog")

  # unix system/architecture
  string (TOLOWER "${CMAKE_SYSTEM_NAME}" CPACK_SYSTEM_NAME)
  if (CMAKE_CXX_FLAGS MATCHES "-m32")
    set (CPACK_PACKAGE_ARCHITECTURE i386)
  else ()
    execute_process (
      COMMAND         dpkg --print-architecture
      RESULT_VARIABLE RV
      OUTPUT_VARIABLE CPACK_PACKAGE_ARCHITECTURE
    )
    if (RV EQUAL 0)
      string (STRIP "${CPACK_PACKAGE_ARCHITECTURE}" CPACK_PACKAGE_ARCHITECTURE)
    else ()
      execute_process (
        COMMAND uname -m 
        OUTPUT_VARIABLE CPACK_PACKAGE_ARCHITECTURE
      )
      if (CPACK_PACKAGE_ARCHITECTURE MATCHES "x86_64")
        set (CPACK_PACKAGE_ARCHITECTURE amd64)
      else ()
        set (CPACK_PACKAGE_ARCHITECTURE i386)
      endif ()
    endif ()
  endif ()

  # source package settings
  set (CPACK_SOURCE_TOPLEVEL_TAG      "source")
  set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
  set (CPACK_SOURCE_IGNORE_FILES      "/\\\\.git/;\\\\.swp$;\\\\.#;/#;\\\\.*~;cscope\\\\.*;/[Bb]uild[.+-_a-zA-Z0-9]*/")

  # default binary package settings
  set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY TRUE)
  set (CPACK_PACKAGE_FILE_NAME          "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
  if (CPACK_PACKAGE_ARCHITECTURE)
    set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-${CPACK_PACKAGE_ARCHITECTURE}")
  endif ()
  
  # generator specific configuration file
  #
  # allow package maintainers to use their own configuration file
  # $ cmake -DCPACK_PROJECT_CONFIG_FILE:FILE=/path/to/package/config
  if (NOT CPACK_PROJECT_CONFIG_FILE)
    configure_file (
      cmake/templates/package.cmake.in
      "${PROJECT_BINARY_DIR}/${PACKAGE_NAME}-package.cmake" @ONLY
    )
    set (CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/${PACKAGE_NAME}-package.cmake")
  endif ()

  include (CPack)

endif (BUILD_PACKAGING) # BUILD_PACKAGING