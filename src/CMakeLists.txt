
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
target_link_libraries(${PROJECT_NAME} fuse glog gflags -lpthread qingstor)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

#
# make libs referenced by unit test
#
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
add_library(
  qsfsConfigure ${LIBTYPE}
  ${QSFS_CONFIGURE_SRCS}
  )
target_link_libraries(qsfsConfigure fuse)
set_target_properties(qsfsConfigure PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsConfigure PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

add_library(
  qsfsLogging ${LIBTYPE}
  base/Logging.cpp 
  base/LogLevel.cpp
  )
target_link_libraries(qsfsLogging glog qsfsConfigure)
set_target_properties(qsfsLogging PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsLogging PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

add_library(
  qsfsBaseUtils ${LIBTYPE}
  base/StringUtils.cpp
  base/TimeUtils.cpp
  base/Utils.cpp
  )
target_link_libraries(qsfsBaseUtils qsfsLogging)
set_target_properties(qsfsBaseUtils PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsBaseUtils PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

add_library(
  qsfsThreadPool ${LIBTYPE}
  base/ThreadPool.cpp
  base/TaskHandle.cpp
)
target_link_libraries(qsfsThreadPool)
set_target_properties(qsfsThreadPool PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsThreadPool PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

add_library(
  qsfsDirectory ${LIBTYPE}
  data/Directory.cpp 
  data/FileMetaData.cpp
  data/FileMetaDataManager.cpp
  )
target_link_libraries(qsfsDirectory)
set_target_properties(qsfsDirectory PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsDirectory PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

add_library(
  qsfsCache ${LIBTYPE}
  data/Cache.cpp
  data/File.cpp
  data/Page.cpp
  )
target_link_libraries(qsfsCache)
set_target_properties(qsfsCache PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsCache PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

add_library(
  qsfsResource ${LIBTYPE}
  data/IOStream.cpp
  data/ResourceManager.cpp
  data/StreamBuf.cpp
  data/StreamUtils.cpp
  )
target_link_libraries(qsfsResource)
set_target_properties(qsfsResource PROPERTIES VERSION ${QSFS_MAJOR_VERSION})
if (BUILD_SHARED_LIBS)
  set_target_properties(qsfsResource PROPERTIES SOVERSION ${QSFS_VERSION})
endif (BUILD_SHARED_LIBS)

#
# install
#
install (
  TARGETS 
  ${PROJECT_NAME}
  RUNTIME DESTINATION bin
)

