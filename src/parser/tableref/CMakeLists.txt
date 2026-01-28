add_library(chickenDB_parser_tableref OBJECT base_tableref.cpp cross_product_ref.cpp join_ref.cpp)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_parser_tableref> PARENT_SCOPE)