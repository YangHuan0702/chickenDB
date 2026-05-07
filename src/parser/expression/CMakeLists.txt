add_library(chicken_parser_expression OBJECT transformer_binary_expression.cpp transformer_column_expression.cpp transformer_constant_expression.cpp transformer_expression.cpp transformer_in_expression.cpp
        transformer_unary_expression.cpp transformer_star.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chicken_parser_expression> PARENT_SCOPE)