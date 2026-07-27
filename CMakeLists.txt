cmake_minimum_required(VERSION 3.16)
project(digitizer-api-example VERSION 1.0.0)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(FETCHCONTENT_QUIET OFF)

set(DIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libs"
    CACHE PATH "Root directory of prebuilt third-party libraries")
set(DIGITIZER_API_EXAMPLES_API_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/digitizer-api"
    CACHE PATH "Root directory of the prebuilt Digitizer API")

set(absl_DIR "${DIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT}/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/absl")
set(utf8_range_DIR "${DIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT}/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/utf8_range")
set(protobuf_DIR "${DIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT}/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/protobuf")
set(gRPC_DIR "${DIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT}/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/grpc")

if(NOT TARGET valijson)
    add_library(valijson INTERFACE)
    target_compile_definitions(valijson INTERFACE VALIJSON_USE_EXCEPTIONS=1)
    if(WIN32)
        target_include_directories(valijson INTERFACE
            "${DIGITIZER_API_EXAMPLES_API_ROOT}/win64/${CMAKE_BUILD_TYPE}/include/valijson")
    else()
        target_include_directories(valijson INTERFACE
            "${DIGITIZER_API_EXAMPLES_API_ROOT}/linux/${CMAKE_BUILD_TYPE}/include/valijson")
    endif()
endif()

if(WIN32)

set(digiscope-api_DIR "${DIGITIZER_API_EXAMPLES_API_ROOT}/win64/${CMAKE_BUILD_TYPE}/lib/cmake/")

find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

find_package(digiscope-api)

elseif(LINUX)

set(digiscope-api_DIR "${DIGITIZER_API_EXAMPLES_API_ROOT}/linux/${CMAKE_BUILD_TYPE}/share/")

find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

find_package(digiscope-api)

endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network StateMachine)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
