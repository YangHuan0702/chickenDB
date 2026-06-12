add_executable(chickendb main.cpp)
target_link_libraries(chickendb chickenDB)
set_target_properties(chickendb PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
