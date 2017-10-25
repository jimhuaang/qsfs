#
# install headers
#

file(GLOB QSFS_BASE_HEADERS "base/*.h")
file(GLOB QSFS_CLIENT_HEADERS "client/*.h")
file(GLOB QSFS_CONFIGURE_HEADERS "configure/*.h")
file(GLOB QSFS_DATA_HEADERS "data/*.h")
file(GLOB QSFS_FILESYSTEM_HEADERS "filesystem/*.h")

install (
  FILES 
  ${QSFS_BASE_HEADERS} 
  DESTINATION include/${PROJECT_NAME}/base
  )

install (
  FILES 
  ${QSFS_CLIENT_HEADERS} 
  DESTINATION include/${PROJECT_NAME}/client
  )

install (
FILES 
  ${QSFS_CONFIGURE_HEADERS} 
  DESTINATION include/${PROJECT_NAME}/configure
  )

install (
  FILES 
  ${QSFS_DATA_HEADERS} 
  DESTINATION include/${PROJECT_NAME}/data
  )

install (
  FILES 
  ${QSFS_FILESYSTEM_HEADERS} 
  DESTINATION include/${PROJECT_NAME}/filesystem
  )