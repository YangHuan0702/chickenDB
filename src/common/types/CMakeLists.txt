add_library(chickenDB_common_types OBJECT value.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_common_types> PARENT_SCOPE)
