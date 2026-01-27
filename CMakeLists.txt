cmake_minimum_required(VERSION 3.10)
project(chickenDB
LANGUAGES C CXX
VERSION 2026.1.26)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_compile_options("-fexceptions")
add_link_options("-fexceptions")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if (${CMAKE_CXX_COMPILER_VERSION} MATCHES "^12.")
        message(STATUS "you use the ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
    else ()
        message(WARNING "haloTP need using compiler clang version >= 12.0")
    endif ()
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
    message(STATUS "you use the ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
else ()
    message(WARNING "Project Need using for Compile Clang Version >= 12.0")
endif ()


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#message(STATUS "CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-parameter -Wno-attributes")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(PROJECT_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/src/include")
set(PROJECT_TEST_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/test/include")
set(PROJECT_THIRD_PARTY_INCLUDE_PATH
        "${PROJECT_SOURCE_DIR}/third_party/libpg_query/include"
        "${PROJECT_SOURCE_DIR}/third_party/sql-parser/src"
        "${PROJECT_SOURCE_DIR}/third_party/jsoncpp/include"
)

include_directories(${PROJECT_INCLUDE_PATH} ${PROJECT_TEST_INCLUDE_PATH} ${PROJECT_THIRD_PARTY_INCLUDE_PATH})
include_directories(BEFORE src)

function(disable_target_warnings NAME)
    target_compile_options(${NAME} PRIVATE "-w")
endfunction()


add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(test)
