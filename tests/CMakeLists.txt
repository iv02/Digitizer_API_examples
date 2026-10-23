add_subdirectory(config_file_test)
add_subdirectory(zero_column_test)
add_subdirectory(dependency_test)

if(DIGITIZER_API_EXAMPLES_ENABLE_HARDWARE_TESTS)
    add_subdirectory(hardware)
endif()
