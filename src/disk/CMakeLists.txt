add_library(CHICKEN_DISK OBJECT table_manager.cpp file_lru_manager.cpp file_desc.cpp posix_disk_manager.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_DISK> PARENT_SCOPE)