add_library(CHICKEN_TRANSACTION OBJECT transaction_manager.cpp version_store.cpp log_manager.cpp recovery_manager.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:CHICKEN_TRANSACTION> PARENT_SCOPE)
