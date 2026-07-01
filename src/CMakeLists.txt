add_subdirectory(parser)
add_subdirectory(common)
add_subdirectory(catalog)
add_subdirectory(disk)
add_subdirectory(binder)
add_subdirectory(planner)
add_subdirectory(optimizer)
add_subdirectory(buffer)
add_subdirectory(executor)
add_subdirectory(index)
add_subdirectory(transaction)

add_library(chickenDB STATIC ${ALL_OBJECT_FILES})

set(CHICKEN_LIB
        CHICKEN_PARSER
        CHICKEN_COMMON
        CHICKEN_CATALOG
        CHICKEN_DISK
        CHICKEN_BINDER
        CHICKEN_PLANNER
        CHICKEN_OPTIMIZER
        CHICKEN_OPTIMIZER_RULES
        CHICKEN_BUFFER
        CHICKEN_EXECUTOR
        CHICKEN_INDEX
        CHICKEN_TRANSACTION
)

find_package(Threads REQUIRED)
get_target_property(_glog_type glog::glog TYPE)
message(STATUS "glog::glog type = ${_glog_type}")
set(CHICKEN_THIRD_PARTY_LIBS duckdb_pg_query sql_parser glog::glog libzstd_static Threads::Threads)

target_link_libraries(chickenDB ${CHICKEN_THIRD_PARTY_LIBS} ${CHICKEN_LIB})
target_include_directories(chickenDB PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# 交互式命令行（在 chickenDB 库定义之后注册，以便链接它）。
add_subdirectory(cli)
