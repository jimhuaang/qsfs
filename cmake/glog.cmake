if (CMAKE_VERSION VERSION_LESS 3.2)
  set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
  set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include(cmake/DownloadProject.cmake)

# Use googlelog instead of glog to avoid override the CMAKE implict variables
# glog_SOURCE_DIR and glog_BINARY_DIR
download_project(PROJ            googlelog
             GIT_REPOSITORY      https://github.com/google/glog.git
             GIT_TAG             master
             ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${googlelog_SOURCE_DIR} ${googlelog_BINARY_DIR})

# Explicitly add required path dependencies
# Where glog headers can be found.
include_directories( ${glog_BINARY_DIR}) #glog/logging.h
include_directories( ${glog_SOURCE_DIR}/src) #glog/log_severity.h
# Where glog targets can be found.
link_directories(${glog_BINARY_DIR})