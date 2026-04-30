add_subdirectory(statement)

add_library(CHICKEN_PARSER OBJECT parser.cpp transformer.cpp)
target_link_libraries(CHICKEN_PARSER PUBLIC glog::glog)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_PARSER> PARENT_SCOPE)