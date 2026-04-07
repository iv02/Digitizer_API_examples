project(config_file_test)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test Concurrent StateMachine)

set(CONFIG_FILE_TEST_SOURCES tst_config_file_test.cpp)

add_executable(${PROJECT_NAME} ${CONFIG_FILE_TEST_SOURCES})
add_test(
    NAME ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E env QTEST_FUNCTION_TIMEOUT=100000000 $<TARGET_FILE:${PROJECT_NAME}>
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties(${PROJECT_NAME} PROPERTIES TIMEOUT 100000)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "tests")

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/digitizer-api/win64/${CMAKE_BUILD_TYPE}/include/event-packet/frameparse"
    )
elseif(LINUX)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/digitizer-api/linux/${CMAKE_BUILD_TYPE}/include/event-packet/frameparse"
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Test
        Qt6::Concurrent
        Qt6::StateMachine
        digiscope-api::digitizer-wrapper
        digiscope-api::event-packet
)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
            --verbose 2
            "$<TARGET_FILE:${PROJECT_NAME}>"
            USES_TERMINAL
        )
    endif()
endif()
