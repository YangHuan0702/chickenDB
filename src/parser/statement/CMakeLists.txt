add_library(chickenDB_parser_statement OBJECT select_statement.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_parser_statement> PARENT_SCOPE)
