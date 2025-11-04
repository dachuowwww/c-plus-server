file(GLOB_RECURSE PINE_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Add custom targets for building and running tests
add_custom_target(build_tests COMMAND ${CMAKE_CTEST_COMMAND} --show-only)
add_custom_target(check_tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

foreach(pine_test_source ${PINE_TEST_SOURCES})
    get_filename_component(pine_test_filename ${pine_test_source} NAME)
    string(REPLACE ".cpp" "" pine_test_name ${pine_test_filename})

    # Add test executable but exclude it from the build by default,only build when make that target
    add_executable(${pine_test_name} EXCLUDE_FROM_ALL ${pine_test_source})

    #depend on all tests
    add_dependencies(build_tests ${pine_test_name})
    add_dependencies(check_tests ${pine_test_name})
    target_include_directories(${pine_test_name} PRIVATE ${PINE_INCLUDE_DIR})
    target_link_libraries(${pine_test_name} PINE_SHARED)
    # Set output directory for test executables,cuz global output directory is not suitable for test executables
    set_target_properties(${pine_test_name} 
        PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    add_test(NAME ${pine_test_name} 
    COMMAND ${pine_test_name} 
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()
