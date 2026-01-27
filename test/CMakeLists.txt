cmake_minimum_required(VERSION 3.16)
enable_testing()
include(GoogleTest)

file(GLOB_RECURSE HALOTP_FULL_TEST_SOURCE_FILES "${PROJECT_SOURCE_DIR}/test/*/*test.cpp")

foreach (halotp_test_sources_file ${HALOTP_FULL_TEST_SOURCE_FILES})
    get_filename_component(halotp_test_filename ${halotp_test_sources_file} NAME)
    string(REPLACE ".cpp" "" halotp_test_name ${halotp_test_filename})
    add_executable(${halotp_test_name} EXCLUDE_FROM_ALL ${halotp_test_sources_file})
    target_link_libraries(${halotp_test_name} gtest gmock_main chickenDB)
    set_target_properties(${halotp_test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test" COMMAND ${halotp_test_name})
    add_test(NAME ${halotp_test_name} COMMAND ${halotp_test_name})
endforeach ()