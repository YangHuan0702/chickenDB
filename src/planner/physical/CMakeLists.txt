add_subdirectory(operator)

add_library(CHICKEN_PLANNER_PHYSICAL OBJECT physical_aggregate_operator.cpp physical_creata_table.cpp physical_delete.cpp physical_filter_operator.cpp
        physical_insert.cpp physical_join_operator.cpp physical_limit_operator.cpp physical_project_operator.cpp physical_scan_operator.cpp physical_sort_operator.cpp
        physical_update.cpp physical_distinct_operator.cpp physical_stream_aggregate_operator.cpp)


set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_PLANNER_PHYSICAL> PARENT_SCOPE)