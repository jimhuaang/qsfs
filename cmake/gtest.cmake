if (CMAKE_VERSION VERSION_LESS 3.2)
set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadProject.cmake)

# Use googletest instead of gtest to avoid override the CMAKE implict variables
# gtest_SOURCE_DIR and gtest_BINARY_DIR
download_project(PROJ            googletest
             GIT_REPOSITORY      https://github.com/google/googletest.git
             GIT_TAG             master
             ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR}) 

# Explicitly add required path dependencies
# Where gtest headers can be found.
include_directories( ${gtest_SOURCE_DIR}/include)
# Where gtest targets can be found.
link_directories(${gtest_BINARY_DIR})
