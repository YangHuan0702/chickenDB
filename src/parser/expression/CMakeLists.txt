add_library(chickenDB_parser_expression OBJECT column_expression.cpp comparison_expression.cpp
        conjunction_expression.cpp default_expression.cpp star_expression.cpp content_expression.cpp case_expression.cpp cast_expression.cpp function_expression.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_parser_expression> PARENT_SCOPE)
