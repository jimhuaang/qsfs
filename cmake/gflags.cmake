if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadInstallProject.cmake)

# Use googleflags instead of gflags to avoid override the CMAKE implict variables
# gflags_SOURCE_DIR and gflags_BINARY_DIR
setup_download_project(PROJ    googleflags
           GIT_REPOSITORY      https://github.com/gflags/gflags.git
           GIT_TAG             master
           ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Download
download_project(googleflags)

# Install
install_project(googleflags ${EXTERNAL_PROJECT_INSTALL_DIR})

#add_subdirectory(${googleflags_SOURCE_DIR} ${googleflags_BINARY_DIR})

# Explicitly add required path dependencies
# Where gflags headers can be found.
include_directories(BEFORE ${EXTERNAL_PROJECT_INSTALL_DIR}/include})
# Where gflags targets can be found.
link_directories(${EXTERNAL_PROJECT_INSTALL_DIR}/lib)

# Uninstall
include(cmake/UninstallProject.cmake)
setup_uninstall_project(googleflags)