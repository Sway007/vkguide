cmake_minimum_required(VERSION 3.10)

project("vk_guide_sway")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party)

set(APP_NAME "VkRenderApp")
set(LIB_NAME "VkRenderEngine")

add_subdirectory(LibRenderer)
add_subdirectory(SDLApp)

# Create symlink for compile_commands.json
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
    COMMENT "Creating symlink to compile_commands.json"
)
add_custom_target(COMPILE_COMMANDS ALL DEPENDS ${CMAKE_SOURCE_DIR}/compile_commands.json)
