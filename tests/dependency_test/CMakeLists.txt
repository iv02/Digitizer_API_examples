project(dependency_test)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test Concurrent StateMachine)

set(DEPENDENCY_TEST_SOURCES tst_dependency_test.cpp)

add_executable(${PROJECT_NAME} ${DEPENDENCY_TEST_SOURCES})
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "tests")

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${DIGITIZER_API_EXAMPLES_API_ROOT}/win64/${CMAKE_BUILD_TYPE}/include/event-packet/frameparse"
    )
elseif(LINUX)
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${DIGITIZER_API_EXAMPLES_API_ROOT}/linux/${CMAKE_BUILD_TYPE}/include/event-packet/frameparse"
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

