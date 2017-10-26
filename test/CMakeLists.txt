
if (QSFS_BUILD_TESTS)
  add_executable(
    ExceptionTest
    ExceptionTest.cpp
  )
  target_link_libraries(ExceptionTest gtest)
  add_test(NAME qsfs_exception COMMAND ExceptionTest)

  add_executable(
    LoggingTest
    LoggingTest.cpp 
    )
  target_link_libraries(LoggingTest gtest qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_logging COMMAND LoggingTest)

  add_executable(
    ThreadPoolTest 
    ThreadPoolTest.cpp
    )
  target_link_libraries(ThreadPoolTest gtest qsfsThreadPool)
  add_test(NAME qsfs_threadpool COMMAND ThreadPoolTest)

  add_executable(
    DirectoryTest
    DirectoryTest.cpp
    )
  target_link_libraries(DirectoryTest gtest qsfsDirectory qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_directory COMMAND DirectoryTest)

  add_executable(
    FileMetaDataManagerTest
    FileMetaDataManagerTest.cpp
    )
  target_link_libraries(FileMetaDataManagerTest gtest qsfsDirectory qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_metadata_manager COMMAND FileMetaDataManagerTest)

  add_executable(
    StreamTest
    StreamTest.cpp
    )
  target_link_libraries(StreamTest gtest qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_stream COMMAND StreamTest)

  add_executable(
    ResourceManagerTest
    ResourceManagerTest.cpp
    )
  target_link_libraries(ResourceManagerTest gtest qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_resource_manager COMMAND ResourceManagerTest)

  add_executable(
    PageTest
    PageTest.cpp
    )
  target_link_libraries(PageTest gtest qsfsCache qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_page COMMAND PageTest)

  add_executable(
    FileTest
    FileTest.cpp
    )
  target_link_libraries(FileTest gtest qsfsCache qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_file COMMAND FileTest)

  add_executable(
    CacheTest
    CacheTest.cpp
    )
  target_link_libraries(CacheTest gtest qsfsCache qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME qsfs_cache COMMAND CacheTest)

endif (QSFS_BUILD_TESTS)
