add_library(chickenDB_common_enums OBJECT statement_type.cpp join_type.cpp expression_type.cpp)

target_link_libraries(chickenDB_common_enums PUBLIC glog::glog)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_common_enums> PARENT_SCOPE)