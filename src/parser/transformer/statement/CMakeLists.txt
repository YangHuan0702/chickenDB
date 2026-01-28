add_library(chickenDB_parser_transformer_statement OBJECT select_statement.cpp delete_statement.cpp insert_statement.cpp
        update_statement.cpp create_table_statement.cpp create_db_statement.cpp transform_select_node.cpp range_var_statement.cpp
    transform_alias.cpp orderby_transform.cpp from_transform.cpp groupby_transform.cpp expression_list_transform.cpp expression_transform.cpp
    table_ref_node_transform.cpp join_transform.cpp range_subselect_transform.cpp range_function_transform.cpp
)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:chickenDB_parser_transformer_statement> PARENT_SCOPE)
