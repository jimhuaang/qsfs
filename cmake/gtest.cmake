if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadInstallProject.cmake)

# Use googletest instead of gtest to avoid override the CMAKE implict variables
# gtest_SOURCE_DIR and gtest_BINARY_DIR
setup_download_project(PROJ      googletest
             GIT_REPOSITORY      https://github.com/google/googletest.git
             GIT_TAG             master
             ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Download and 
download_project(googletest)

#Install
install_project(googletest ${EXTERNAL_PROJECT_INSTALL_DIR})

#add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR}) 

# Explicitly add required path dependencies
# Where gtest headers can be found.
include_directories(BEFORE ${EXTERNAL_PROJECT_INSTALL_DIR}/include)
# Where gtest targets can be found.
link_directories(${EXTERNAL_PROJECT_INSTALL_DIR}/lib)

# Uninstall
include(cmake/UninstallProject.cmake)
setup_uninstall_project(googletest)
