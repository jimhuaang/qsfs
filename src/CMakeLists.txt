
#
# main program
#
file(GLOB QSFS_SRCS "*.cpp")
file(GLOB QSFS_BASE_SRCS "base/*.cpp")
file(GLOB QSFS_CLIENT_SRCS "client/*.cpp")
file(GLOB QSFS_CONFIGURE_SRCS "configure/*.cpp")
file(GLOB QSFS_DATA_SRCS "data/*.cpp")
file(GLOB QSFS_FILESYSTEM_SRCS "filesystem/*.cpp")
file(GLOB QSFS_SOURCES 
  ${QSFS_SRCS}
  ${QSFS_BASE_SRCS}
  ${QSFS_CLIENT_SRCS}
  ${QSFS_CONFIGURE_SRCS}
  ${QSFS_DATA_SRCS}
  ${QSFS_FILESYSTEM_SRCS}
  )

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(
  ${PROJECT_NAME} 
  ${QSFS_SOURCES}
  )
target_link_libraries(${PROJECT_NAME} fuse glog gflags ${CMAKE_THREAD_LIBS_INIT} qingstor)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

#
# make libs referenced by unit test
#
# use OBJECT lib type, as we do not need to produce any library files
add_library(
  qsfsLogging OBJECT
  base/Logging.cpp 
  base/LogLevel.cpp
  ${QSFS_CONFIGURE_SRCS}
  )

add_library(
  qsfsBaseUtils OBJECT
  base/StringUtils.cpp
  base/TimeUtils.cpp
  base/Utils.cpp
  )

add_library(
  qsfsThreadPool OBJECT
  base/ThreadPool.cpp
  base/TaskHandle.cpp
)

add_library(
  qsfsDirectory OBJECT
  data/Directory.cpp 
  data/FileMetaData.cpp
  data/FileMetaDataManager.cpp
  )

add_library(
  qsfsCache OBJECT
  data/Cache.cpp
  data/File.cpp
  data/Page.cpp
  )

add_library(
  qsfsResource OBJECT
  data/IOStream.cpp
  data/ResourceManager.cpp
  data/StreamBuf.cpp
  data/StreamUtils.cpp
  )

#
# install
#
install (
  TARGETS 
  ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

