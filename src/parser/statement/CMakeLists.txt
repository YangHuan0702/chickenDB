add_library(clickenDB_parser_statement OBJECT transformer_create.cpp transformer_delete.cpp transformer_insert.cpp transformer_select.cpp transformer_update.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:clickenDB_parser_statement> PARENT_SCOPE)