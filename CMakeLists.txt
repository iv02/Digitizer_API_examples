cmake_minimum_required(VERSION 3.16)
project(digitizer-api-example VERSION 1.0.0)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(FETCHCONTENT_QUIET OFF)

if(WIN32)

set(absl_DIR "${CMAKE_SOURCE_DIR}/libs/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/absl")
set(utf8_range_DIR "${CMAKE_SOURCE_DIR}/libs/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/utf8_range")
set(protobuf_DIR "${CMAKE_SOURCE_DIR}/libs/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/protobuf")
set(gRPC_DIR "${CMAKE_SOURCE_DIR}/libs/grpc/${CMAKE_BUILD_TYPE}/lib/cmake/grpc")
set(digiscope-api_DIR "${CMAKE_SOURCE_DIR}/digitizer-api/win64/${CMAKE_BUILD_TYPE}/lib/cmake/")

find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

find_package(digiscope-api)

elseif(LINUX)

set(digiscope-api_DIR "${CMAKE_SOURCE_DIR}/digitizer-api/linux/${CMAKE_BUILD_TYPE}/share/")

find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

find_package(digiscope-api)

endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network StateMachine)

add_subdirectory(src)
