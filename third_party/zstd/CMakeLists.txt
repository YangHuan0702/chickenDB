# Minimal vendored build of zstd (v1.5.6) for chickenDB.
# We only need the static library for in-process compress/decompress of page
# column blocks, so we skip zstd's own CLI / tests / dictionary-training tools
# and the hand-written x86_64 assembly (disabled via ZSTD_DISABLE_ASM) to keep
# the build portable across Clang/MinGW on Windows.

file(GLOB ZSTD_COMMON_SOURCES     "${CMAKE_CURRENT_SOURCE_DIR}/lib/common/*.c")
file(GLOB ZSTD_COMPRESS_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/lib/compress/*.c")
file(GLOB ZSTD_DECOMPRESS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lib/decompress/*.c")

add_library(libzstd_static STATIC
        ${ZSTD_COMMON_SOURCES}
        ${ZSTD_COMPRESS_SOURCES}
        ${ZSTD_DECOMPRESS_SOURCES})

target_include_directories(libzstd_static PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# Single-threaded, no assembly: keeps the build self-contained and portable.
target_compile_definitions(libzstd_static PRIVATE
        ZSTD_DISABLE_ASM=1
        ZSTD_MULTITHREAD=0)

# zstd ships with its own warning hygiene; silence it like the other vendored
# libs so it does not trip chickenDB's -Werror.
disable_target_warnings(libzstd_static)
