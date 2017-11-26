if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadInstallProject.cmake)

setup_download_project(PROJ    qingstorsdk
           GIT_REPOSITORY      https://github.com/jimhuaang/qsfs-sdk-cpp.git
           GIT_TAG             master
           ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

# Download and Install
download_install_project(qingstorsdk)

# Uninstall
include(cmake/UninstallProject.cmake)
setup_uninstall_project(qingstorsdk)