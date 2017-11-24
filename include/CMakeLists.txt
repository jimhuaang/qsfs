#
# install headers
#

file(GLOB QSFS_BASE_HEADERS "base/*.h")
file(GLOB QSFS_CLIENT_HEADERS "client/*.h")
file(GLOB QSFS_CONFIGURE_HEADERS "configure/*.h")
file(GLOB QSFS_DATA_HEADERS "data/*.h")
file(GLOB QSFS_FILESYSTEM_HEADERS "filesystem/*.h")

if (INSTALL_HEADERS)
  install (
    FILES 
    ${QSFS_BASE_HEADERS} 
    DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}/base COMPONENT dev
    )

  install (
    FILES 
    ${QSFS_CLIENT_HEADERS} 
    DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}/client COMPONENT dev
    )

  install (
    FILES 
    ${QSFS_CONFIGURE_HEADERS} 
    DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}/configure COMPONENT dev
    )

  install (
    FILES 
    ${QSFS_DATA_HEADERS} 
    DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}/data COMPONENT dev
    )

  install (
    FILES 
    ${QSFS_FILESYSTEM_HEADERS} 
    DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}/filesystem COMPONENT dev
    )
endif ()