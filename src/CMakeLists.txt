add_subdirectory(parser)

add_library(haloTP STATIC ${ALL_OBJECT_FILES})

set(haloTP_LIB
        HALOTP_PARSER)

find_package(Threads REQUIRED)

set(haloTP_THIRD_PARTY_LIBS duckdb_pg_query sql_parser Threads::Threads)

target_link_libraries(haloTP ${haloTP_THIRD_PARTY_LIBS} ${haloTP_LIB})

target_link_directories(haloTP PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
