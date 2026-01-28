add_library(chickenDB_parser_query_node OBJECT select_node.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_parser_query_node> PARENT_SCOPE)
