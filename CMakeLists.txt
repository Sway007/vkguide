cmake_minimum_required(VERSION 3.13) # fastgltf requires 3.13

project("vk_guide_sway")

# Include FetchContent module
include(FetchContent)

# Fetch fastgltf
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG main # 或者使用特定版本标签，如 "v0.9.0"
)

# Set fastgltf options (disable tests and examples for faster build)
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)

# Make fastgltf available
FetchContent_MakeAvailable(fastgltf)

set(LIB_DIST_DIR ${CMAKE_CURRENT_LIST_DIR}/dist)
set(SHADER_DIST_DIR ${LIB_DIST_DIR}/shaders)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIST_DIR})
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

# Compile shaders
find_program(GLSL_COMPILER glslc REQUIRED)
message(STATUS "GLSL_COMPILER: ${GLSL_COMPILER}")
file(GLOB_RECURSE SHADER_SOURCES ${CMAKE_CURRENT_LIST_DIR}/shaders/*.comp ${CMAKE_CURRENT_LIST_DIR}/shaders/*.vert ${CMAKE_CURRENT_LIST_DIR}/shaders/*.frag)
foreach(shader ${SHADER_SOURCES})
    get_filename_component(shader_name ${shader} NAME)
    set(SHADER_OUTPUT ${SHADER_DIST_DIR}/${shader_name}.spv)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DIST_DIR}
        COMMAND ${GLSL_COMPILER} ${shader} -o ${SHADER_OUTPUT}
        DEPENDS ${shader}
        COMMENT "Compiling ${shader} to SPIR-V"
    )
    list(APPEND SPV_FILES ${SHADER_OUTPUT})
endforeach()
add_custom_target(compile_shaders ALL DEPENDS ${SPV_FILES})

