#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "digiscope-api::protobuf-api" for configuration "Release"
set_property(TARGET digiscope-api::protobuf-api APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(digiscope-api::protobuf-api PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libprotobuf-api.a"
  )

list(APPEND _cmake_import_check_targets digiscope-api::protobuf-api )
list(APPEND _cmake_import_check_files_for_digiscope-api::protobuf-api "${_IMPORT_PREFIX}/lib/libprotobuf-api.a" )

# Import target "digiscope-api::event-packet" for configuration "Release"
set_property(TARGET digiscope-api::event-packet APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(digiscope-api::event-packet PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libevent-packet.a"
  )

list(APPEND _cmake_import_check_targets digiscope-api::event-packet )
list(APPEND _cmake_import_check_files_for_digiscope-api::event-packet "${_IMPORT_PREFIX}/lib/libevent-packet.a" )

# Import target "digiscope-api::networker" for configuration "Release"
set_property(TARGET digiscope-api::networker APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(digiscope-api::networker PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libnetworker.a"
  )

list(APPEND _cmake_import_check_targets digiscope-api::networker )
list(APPEND _cmake_import_check_files_for_digiscope-api::networker "${_IMPORT_PREFIX}/lib/libnetworker.a" )

# Import target "digiscope-api::digitizer-wrapper" for configuration "Release"
set_property(TARGET digiscope-api::digitizer-wrapper APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(digiscope-api::digitizer-wrapper PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libdigitizer-wrapper.a"
  )

list(APPEND _cmake_import_check_targets digiscope-api::digitizer-wrapper )
list(APPEND _cmake_import_check_files_for_digiscope-api::digitizer-wrapper "${_IMPORT_PREFIX}/lib/libdigitizer-wrapper.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
