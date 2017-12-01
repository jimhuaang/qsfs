if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadInstallProject.cmake)

setup_download_project(PROJ    qingstorsdk
           GIT_REPOSITORY      https://github.com/jimhuaang/qsfs-sdk-cpp.git
           GIT_TAG             qsfsStable
           ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Download and 
download_project(qingstorsdk)

#Install
#install_project(qingstorsdk ${EXTERNAL_PROJECT_INSTALL_DIR})

add_subdirectory(${qingstorsdk_SOURCE_DIR})

include_directories(${qingstorsdk_SOURCE_DIR}/include)

#link_directories(${qingstorsdk_BINARY_DIR}/bin)
link_directories(${CMAKE_BINARY_DIR}/build/qingstorsdk/src/bin)

# as qingstorsdk is add as subdirectory of qsfs, so no need to uninstall individually
# Uninstall
#include(cmake/UninstallProject.cmake)
#setup_uninstall_project(qingstorsdk)