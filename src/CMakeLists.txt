add_subdirectory(parser)
add_subdirectory(common)

add_library(chickenDB STATIC ${ALL_OBJECT_FILES})

set(CHICKEN_LIB
        CHICKEN_PARSER)

find_package(Threads REQUIRED)
get_target_property(_glog_type glog::glog TYPE)
message(STATUS "glog::glog type = ${_glog_type}")
set(CHICKEN_THIRD_PARTY_LIBS duckdb_pg_query sql_parser glog::glog Threads::Threads)

target_link_libraries(chickenDB ${CHICKEN_THIRD_PARTY_LIBS} ${CHICKEN_LIB})
target_include_directories(chickenDB PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
