add_library(CHICKEN_BUFFER OBJECT buffer_manager.cpp clock_sweep_lru.cpp table_data_page.cpp table_scan_iterator.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_BUFFER> PARENT_SCOPE)
