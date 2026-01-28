add_library(chickenDB_parser_tableref OBJECT base_tableref.cpp cross_product_ref.cpp join_ref.cpp subquery_transform.cpp sub_query_ref.cpp
        table_function_transform.cpp table_function.cpp )
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_parser_tableref> PARENT_SCOPE)