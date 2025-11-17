cmake_minimum_required(VERSION 3.16)
project(digitizer-example)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent StateMachine)

file(GLOB HEADERS CONFIGURE_DEPENDS "*.h")
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "src" FILES ${HEADERS} ${SOURCES})

if(NOT SOURCES)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "src" FILES ${HEADERS} ${SOURCES})

source_group("src" FILES ${HEADERS} ${SOURCES})

qt_add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE QT_MESSAGELOGCONTEXT)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Gui
    Qt6::Widgets 
    Qt6::Concurrent
    Qt6::StateMachine 
    digiscope-api::digitizer-wrapper
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
