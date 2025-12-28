find_package(Vulkan REQUIRED)

set(VULKAN_SDK_DIR "~/VulkanSDK/1.4.328.1/macOS/include")

file(GLOB_RECURSE LIB_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
set(LIB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include PARENT_SCOPE)

file(GLOB IM_GUI_SOURCES ${THIRD_PARTY_DIR}/imgui/*.cpp ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_vulkan.cpp ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl3.cpp)

add_library(${LIB_NAME} ${LIB_SOURCES} ${IM_GUI_SOURCES})

target_compile_definitions(${LIB_NAME} PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
target_include_directories(${LIB_NAME} PUBLIC ${THIRD_PARTY_DIR}/imgui ${THIRD_PARTY_DIR}/imgui/backends ${Vulkan_INCLUDE_DIR} ${VULKAN_SDK_DIR})
target_link_libraries(${LIB_NAME} Vulkan::Vulkan)
target_compile_definitions(${LIB_NAME} PRIVATE SHADER_DIR="${SHADER_DIST_DIR}")
