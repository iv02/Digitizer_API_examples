function(add_hardware_test test_name)
    set(target "hardware_${test_name}")

    add_executable(${target} "${test_name}/tst_${test_name}.cpp")
    add_test(NAME ${target} COMMAND ${target})
    set_property(TARGET ${target} PROPERTY FOLDER "tests/hardware")

    if(WIN32 OR LINUX)
        target_include_directories(${target} PRIVATE
            "${DIGITIZER_API_EXAMPLES_API_ROOT}/${CMAKE_BUILD_TYPE}/include/event-packet/frameparse"
        )
    endif()

    target_link_libraries(${target}
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
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --verbose 2
                "$<TARGET_FILE:${target}>"
                USES_TERMINAL
            )
        endif()
    endif()
endfunction()

add_hardware_test(config_file_test)
add_hardware_test(zero_column_test)
add_hardware_test(dependency_test)
