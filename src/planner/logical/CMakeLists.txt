add_library(CHICKEN_PLANNER_LOGICAL OBJECT logical_create_planner.cpp logical_delete_planner.cpp
        logical_insert_planner.cpp logical_select_planner.cpp logical_update_planner.cpp logical_operator_filter.cpp logical_operator_project.cpp
        logical_operator_scan.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_PLANNER_LOGICAL> PARENT_SCOPE)
