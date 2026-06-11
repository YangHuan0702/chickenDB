add_library(CHICKEN_EXECUTOR OBJECT chunk.cpp execution.cpp execute_create.cpp execute_insert.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_EXECUTOR> PARENT_SCOPE)