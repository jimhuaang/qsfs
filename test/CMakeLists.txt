
if (BUILD_TESTING)

  add_executable(
    ExceptionTest
    ExceptionTest.cpp
  )
  target_link_libraries(ExceptionTest gtest)
  add_test(NAME exception_test COMMAND ExceptionTest)

  add_executable(
    LoggingTest
    LoggingTest.cpp 
    )
  target_link_libraries(LoggingTest gtest qsfsLogging qsfsBaseUtils)
  add_test(NAME logging_test COMMAND LoggingTest)

  add_executable(
    ThreadPoolTest 
    ThreadPoolTest.cpp
    )
  target_link_libraries(ThreadPoolTest gtest qsfsThreadPool)
  add_test(NAME threadpool_test COMMAND ThreadPoolTest)

  add_executable(
    DirectoryTest
    DirectoryTest.cpp
    )
  target_link_libraries(DirectoryTest gtest qsfsDirectory qsfsLogging qsfsBaseUtils)
  add_test(NAME directory_test COMMAND DirectoryTest)

  add_executable(
    FileMetaDataManagerTest
    FileMetaDataManagerTest.cpp
    )
  target_link_libraries(FileMetaDataManagerTest gtest qsfsDirectory qsfsLogging qsfsBaseUtils)
  add_test(NAME metadata_manager_test COMMAND FileMetaDataManagerTest)

  add_executable(
    StreamTest
    StreamTest.cpp
    )
  target_link_libraries(StreamTest gtest qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME stream_test COMMAND StreamTest)

  add_executable(
    ResourceManagerTest
    ResourceManagerTest.cpp
    )
  target_link_libraries(ResourceManagerTest gtest qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME resource_manager_test COMMAND ResourceManagerTest)

  add_executable(
    PageTest
    PageTest.cpp
    )
  target_link_libraries(PageTest gtest qsfsCache qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME page_test COMMAND PageTest)

  add_executable(
    FileTest
    FileTest.cpp
    )
  target_link_libraries(FileTest gtest qsfsCache qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME file_test COMMAND FileTest)

  add_executable(
    CacheTest
    CacheTest.cpp
    )
  target_link_libraries(CacheTest gtest qsfsCache qsfsResource qsfsLogging qsfsBaseUtils)
  add_test(NAME cache_test COMMAND CacheTest)

endif (BUILD_TESTING)
