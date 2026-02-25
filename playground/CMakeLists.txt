cmake_minimum_required(VERSION 3.13) # fastgltf requires 3.13

project("vk_guide_sway/playground" LANGUAGES CXX)

# 导出编译命令数据库
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(cpp_json_parse playground.cpp model.hpp)
target_link_libraries(cpp_json_parse PRIVATE nlohmann_json::nlohmann_json)

# 创建 compile_commands.json 的符号链接到源码目录（方便 VSCode 识别）
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
    COMMENT "Creating symlink to compile_commands.json"
)
add_custom_target(COMPILE_COMMANDS ALL DEPENDS ${CMAKE_SOURCE_DIR}/compile_commands.json)
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}")
