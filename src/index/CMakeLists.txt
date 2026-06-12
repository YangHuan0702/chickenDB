add_library(CHICKEN_INDEX OBJECT b_plus_tree.cpp hash_index.cpp bitmap_index.cpp index_factory.cpp disk_b_plus_tree.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_INDEX> PARENT_SCOPE)
