
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was digiscope-api-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

####################################################################################
include(CMakeFindDependencyMacro)

if("ON" STREQUAL "ON")
    find_dependency(absl REQUIRED)
    find_dependency(utf8_range REQUIRED)
    find_package(protobuf CONFIG REQUIRED)
    find_dependency(gRPC REQUIRED)
else()
    find_package(protobuf CONFIG REQUIRED)
    find_dependency(gRPC REQUIRED)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/digiscope-api-targets.cmake")


# [Variables]
set(DIGISCOPE_API_FOUND TRUE)
set(DIGISCOPE_API_VERSION 1.0.0)

if("" STREQUAL "ON")
    set(DIGISCOPE_API_HAS_PREBUILD_GRPC TRUE)
else()
    set(DIGISCOPE_API_HAS_PREBUILD_GRPC FALSE)
endif()

set(DIGISCOPE_API_HAS_EVENT_PACKET TRUE)

if("" STREQUAL "ON" OR
   "ON" STREQUAL "ON")
        set(DIGISCOPE_API_HAS_NETWORKER TRUE)
else()
        set(DIGISCOPE_API_HAS_NETWORKER FALSE)
endif()

if("ON" STREQUAL "ON")
    set(DIGISCOPE_API_HAS_FW_SETTINGS_WRAPPER TRUE)
else()
    set(DIGISCOPE_API_HAS_FW_SETTINGS_WRAPPER FALSE)
endif()

message(STATUS "Found digiscope-api ${DIGISCOPE_API_VERSION}")
