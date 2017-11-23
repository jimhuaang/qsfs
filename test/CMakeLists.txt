
if (QSFS_BUILD_TESTS)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

  add_executable(
    ExceptionTest
    ExceptionTest.cpp
  )
  target_link_libraries(ExceptionTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_exception COMMAND ExceptionTest)

  add_executable(
    LoggingTest
    LoggingTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    )
  target_link_libraries(LoggingTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_logging COMMAND LoggingTest)

  add_executable(
    ThreadPoolTest 
    ThreadPoolTest.cpp
    $<TARGET_OBJECTS:qsfsThreadPool>
    )
  target_link_libraries(ThreadPoolTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_threadpool COMMAND ThreadPoolTest)

  add_executable(
    DirectoryTest
    DirectoryTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsDirectory>
    )
  target_link_libraries(DirectoryTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_directory COMMAND DirectoryTest)

  add_executable(
    FileMetaDataManagerTest
    FileMetaDataManagerTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsDirectory>
    )
  target_link_libraries(FileMetaDataManagerTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_metadata_manager COMMAND FileMetaDataManagerTest)

  add_executable(
    StreamTest
    StreamTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsResource>
    )
  target_link_libraries(StreamTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_stream COMMAND StreamTest)

  add_executable(
    ResourceManagerTest
    ResourceManagerTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsResource>
    )
  target_link_libraries(ResourceManagerTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_resource_manager COMMAND ResourceManagerTest)

  add_executable(
    PageTest
    PageTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsCache>
    $<TARGET_OBJECTS:qsfsResource>
    )
  target_link_libraries(PageTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_page COMMAND PageTest)

  add_executable(
    FileTest
    FileTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsCache>
    $<TARGET_OBJECTS:qsfsResource>
    )
  target_link_libraries(FileTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_file COMMAND FileTest)

  add_executable(
    CacheTest
    CacheTest.cpp
    $<TARGET_OBJECTS:qsfsLogging>
    $<TARGET_OBJECTS:qsfsBaseUtils>
    $<TARGET_OBJECTS:qsfsCache>
    $<TARGET_OBJECTS:qsfsResource>
    )
  target_link_libraries(CacheTest fuse gtest glog gflags ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_cache COMMAND CacheTest)

endif (QSFS_BUILD_TESTS)