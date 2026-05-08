add_subdirectory(expression)
add_subdirectory(statement)

add_library(CHICKEN_BINDER OBJECT binder.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_BINDER> PARENT_SCOPE)