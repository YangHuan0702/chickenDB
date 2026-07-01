add_library(CHICKEN_OPTIMIZER_RULES OBJECT
        constant_folding.cpp
        predicate_pushdown.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_OPTIMIZER_RULES> PARENT_SCOPE)
