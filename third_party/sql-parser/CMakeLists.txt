# CMake最低版本要求
cmake_minimum_required(VERSION 3.5)

# 设置项目名
project(sql_parser LANGUAGES CXX)


file(GLOB_RECURSE SQL_PARSER_ALL_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp")
file(GLOB_RECURSE SQL_PARSER_ALL_MAIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

message(STATUS "sql_parser all sources : ${SQL_PARSER_ALL_SOURCE_FILES}")
# 创建静态库目标
add_library(sql_parser STATIC ${SQL_PARSER_ALL_MAIN_FILES} ${SQL_PARSER_ALL_SOURCE_FILES})

# 指定头文件搜索路径
target_include_directories(sql_parser
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>  # 如果是当前项目的构建
        $<INSTALL_INTERFACE:include>  # 安装路径下的 include
)

disable_target_warnings(sql_parser)
# 如果需要安装该库
#install(TARGETS sql-parser
#        EXPORT sql-parserConfig
#        LIBRARY DESTINATION lib
#        ARCHIVE DESTINATION lib
#)

# 安装头文件
#install(DIRECTORY include/ DESTINATION include)

# 生成一个供外部项目引用的配置文件（如果需要通过 find_package 引用）
#export(EXPORT sql-parserConfig FILE "${CMAKE_BINARY_DIR}/sql-parserConfig.cmake")
